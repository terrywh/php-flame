#include "consumer.h"
#include "consumer_implement.h"
#include "kafka.h"
#include "../../coroutine.h"
#include "message.h"

namespace flame {
namespace db {
namespace kafka {
	consumer_implement::consumer_implement(consumer* c, php::parameters& params)
	: consumer_(c)
	, worker_()
	, kafka_(nullptr)
	, topic_(nullptr)
	, ctx_(nullptr) {
		if(!params[0].is_array() || !params[1].is_array() || !params[2].is_array()) {
			throw php::exception("failed to create kafka consumer: illegal parameters");
		}
		char   errstr[1024];
		size_t errlen = sizeof(errstr);

		rd_kafka_conf_t* gconf_ = global_conf(params[0]);
		size_t len;
		if(rd_kafka_conf_get(gconf_, "group.id", nullptr, &len) != RD_KAFKA_CONF_OK) {
			rd_kafka_conf_destroy(gconf_);
			throw php::exception("failed to create kafka consumer: global option 'group.id' must be sepecified");
		}
		rd_kafka_conf_set_error_cb(gconf_, error_cb);
		rd_kafka_conf_set_offset_commit_cb(gconf_, offset_commit_cb);
		rd_kafka_conf_set_opaque(gconf_, this);

		rd_kafka_topic_conf_t* tconf_ = rd_kafka_topic_conf_new();
		php::array& tconf = params[1];
		for(auto i=tconf.begin();i!=tconf.end();++i) {
			php::string& name = i->first.to_string();
			php::string& data = i->second.to_string();
			if(RD_KAFKA_CONF_OK != rd_kafka_topic_conf_set(tconf_, name.c_str(), data.c_str(), errstr, errlen)) {
				rd_kafka_conf_destroy(gconf_);
				rd_kafka_topic_conf_destroy(tconf_);
				throw php::exception("failed to set kafka topic conf");
			}
		}
		rd_kafka_conf_set_default_topic_conf(gconf_, tconf_);

		kafka_ = rd_kafka_new(RD_KAFKA_CONSUMER, gconf_, errstr, errlen);
		if(kafka_ == nullptr) {
			rd_kafka_conf_destroy(gconf_);
			throw php::exception(errstr);
		}
		// rd_kafka_poll_set_consumer(kafka_);

		php::array& topics = params[2];
		topic_ = rd_kafka_topic_partition_list_new(topics.length());
		for(auto i=topics.begin(); i!=topics.end(); ++i) {
			php::string& topic = i->second.to_string();
			rd_kafka_topic_partition_list_add(topic_, topic.c_str(), RD_KAFKA_PARTITION_UA);
		}

		rd_kafka_resp_err_t error = rd_kafka_subscribe(kafka_, topic_);
		if(error != RD_KAFKA_RESP_ERR_NO_ERROR) {
			throw php::exception(rd_kafka_err2str(error), error);
		}
	}
	void consumer_implement::error_cb(rd_kafka_t *rk, int err, const char *reason, void *opaque) {
		consumer_implement* self = reinterpret_cast<consumer_implement*>(opaque);
		php::fail("kafka error: (%d) %s", err, reason);
	}
	void consumer_implement::consume1_cb(uv_work_t* handle, int status) {
		consume_request_t* ctx = reinterpret_cast<consume_request_t*>(handle->data);
		if(ctx->rv.is_exception()) {
			ctx->co->fail(ctx->rv);
			delete ctx;
		}else if(ctx->self->ctx_ == nullptr) {
			coroutine::start(ctx->cb, ctx->rv);
			ctx->co->next();
			delete ctx;
		}else{
			coroutine::start(ctx->cb, ctx->rv);
			ctx->self->worker_.queue_work(&ctx->req, ctx->req.work_cb, ctx->req.after_work_cb);
		}
	}
	void consumer_implement::consume1(php::callable& cb) {
		ctx_ = new consume_request_t {
			coroutine::current, this, consumer_, cb
		};
		ctx_->req.data = ctx_;
		worker_.queue_work(&ctx_->req, consume2_wk, consume1_cb);
	}
	void consumer_implement::stop() {
		ctx_ = nullptr;
	}
	void consumer_implement::consume2_wk(uv_work_t* handle) {
		consume_request_t*  ctx = reinterpret_cast<consume_request_t*>(handle->data);
		rd_kafka_message_t* msg;
		do {
			msg = rd_kafka_consumer_poll(ctx->self->kafka_, 10);
		} while(msg == nullptr || msg->err == RD_KAFKA_RESP_ERR__PARTITION_EOF);
		rd_kafka_poll(ctx->self->kafka_, 0);

		if(msg->err == 0) {
			php::object obj = php::object::create<message>();
			message*    cpp = obj.native<message>();
			cpp->init(msg, ctx->self->consumer_);
			ctx->rv = std::move(obj);
		}else{
			ctx->rv = php::make_exception(rd_kafka_err2str(msg->err), msg->err);
		}
	}
	void consumer_implement::consume2_cb(uv_work_t* handle, int status) {
		consume_request_t* ctx = reinterpret_cast<consume_request_t*>(handle->data);
		ctx->co->next(ctx->rv);
		delete ctx;
	}
	void consumer_implement::consume2() {
		consume_request_t* ctx = new consume_request_t {
			coroutine::current, this, consumer_
		};
		ctx->req.data = ctx;
		worker_.queue_work(&ctx->req, consume2_wk, consume2_cb);
	}
	void consumer_implement::commit_wk(uv_work_t* handle) {
		commit_t* ctx = reinterpret_cast<commit_t*>(handle->data);
		rd_kafka_commit_message(ctx->self->kafka_, ctx->msg, true);
		do {
			rd_kafka_poll(ctx->self->kafka_, 1000);
		} while(ctx->rv.is_undefined());
	}
	void consumer_implement::offset_commit_cb(rd_kafka_t *rk, rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t *offsets, void *opaque) {
		consumer_implement* self = reinterpret_cast<consumer_implement*>(opaque);

		if(err == RD_KAFKA_RESP_ERR__NO_OFFSET) {

		}else if(err) {
			std::printf("error: failed to receive offset commit cb (%d)\n", err);
		}else{
			// TODO 优化查询效率
			for(auto i=self->commit_.begin(); i!=self->commit_.end(); /* 可能删除 */) {
				commit_t* ctx = *i;
				rd_kafka_topic_partition_t* part = rd_kafka_topic_partition_list_find(
					offsets, rd_kafka_topic_name(ctx->msg->rkt), ctx->msg->partition);
				if(part == nullptr || part->offset < ctx->msg->offset) ++i;
				else {
					ctx->rv = bool(true);
					i = self->commit_.erase(i);
				}
			}
		}
	}
	void consumer_implement::commit_cb(uv_work_t* handle, int status) {
		commit_t* ctx = reinterpret_cast<commit_t*>(handle->data);
		ctx->co->next(ctx->rv);
		delete ctx;
	}
	void consumer_implement::commit(php::object& msg) {
		commit_t* ctx = new commit_t {
			coroutine::current, this, consumer_, msg.native<message>()->msg_, msg
		};
		ctx->req.data = ctx;
		worker_.queue_work(&ctx->req, commit_wk, commit_cb);
		commit_.push_back(ctx);
	}
	void consumer_implement::close() {
		worker_.close();
		if(ctx_) stop();
		rd_kafka_consumer_close(kafka_);
		while (rd_kafka_outq_len(kafka_) > 0) {
			rd_kafka_poll(kafka_, 1000);
		}

		rd_kafka_topic_partition_list_destroy(topic_);
		rd_kafka_destroy(kafka_);
	}
}
}
}
