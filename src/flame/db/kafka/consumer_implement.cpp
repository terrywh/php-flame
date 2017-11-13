#include "consumer.h"
#include "consumer_implement.h"
#include "kafka.h"
#include "../../coroutine.h"
#include "message.h"

namespace flame {
namespace db {
namespace kafka {
	consumer_implement::consumer_implement(consumer* c, php::parameters& params)
	:consumer_(c) {
		if(!params[0].is_array() || !params[1].is_array() || !params[2].is_array()) {
			throw php::exception("failed to create kafka consumer: illegal parameters");
		}
		char   errstr[1024];
		size_t errlen = sizeof(errstr);

		gconf_ = global_conf(params[0]);
		rd_kafka_conf_set_error_cb(gconf_, error_cb);
		rd_kafka_conf_set_offset_commit_cb(gconf_, commit_cb);
		rd_kafka_conf_set_opaque(gconf_, this);
		kafka_ = rd_kafka_new(RD_KAFKA_PRODUCER, gconf_, errstr, errlen);
		if(kafka_ == nullptr) {
			throw php::exception(errstr);
		}
		php::array& topics = params[2];
		topic_ = rd_kafka_topic_partition_list_new(topics.length());
		for(auto i=topics.begin(); i!=topics.end(); ++i) {
			php::string& topic = i->second.to_string();
			rd_kafka_topic_partition_list_add(topic_, topic.c_str(), RD_KAFKA_PARTITION_UA);
		}
		rd_kafka_subscribe(kafka_, topic_);
		uv_check_init(flame::loop, &check_);
	}
	void consumer_implement::error_cb(rd_kafka_t *rk, int err, const char *reason, void *opaque) {
		consumer_implement* self = reinterpret_cast<consumer_implement*>(opaque);
		php::fail("kafka error: (%d) %s", err, reason);
	}
	typedef struct kafka_request_t {
		coroutine*            co;
		consumer_implement* self;
		php::object          ref;
		php::callable         cb;
	} kafka_request_t;
	void consumer_implement::consume_cb(uv_check_t* handle) {
		kafka_request_t*    ctx = reinterpret_cast<kafka_request_t*>(handle->data);
		rd_kafka_message_t* msg = rd_kafka_consumer_poll(ctx->self->kafka_, 5);
		if(msg == nullptr) return;
		if(msg->err == 0) {
			php::object obj = php::object::create<message>();
			message*    cpp = obj.native<message>();
			cpp->init(msg, ctx->self->consumer_);
			if(ctx->cb.is_null()) {
				ctx->co->next(std::move(obj));
			}else{
				coroutine::start(ctx->cb, obj);
			}
		}else{
			ctx->co->fail(php::make_exception((const char*)msg->payload, msg->err));
		}
		delete ctx;
	}
	void consumer_implement::consume(php::callable& cb) {
		context_ = new kafka_request_t {
			coroutine::current, this, consumer_, cb
		};
		uv_check_stop(&check_);
		check_.data = context_;
		uv_check_start(&check_, consume_cb);
	}
	void consumer_implement::consume() {
		kafka_request_t* ctx = new kafka_request_t {
			coroutine::current, this, consumer_
		};
		uv_check_stop(&check_);
		check_.data = ctx;
		uv_check_start(&check_, consume_cb);
	}
	void consumer_implement::commit_cb(rd_kafka_t *rk, rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t *offsets, void *opaque) {
		consumer_implement* self = reinterpret_cast<consumer_implement*>(opaque);
		for(auto i=self->commits_.begin(); i!=self->commits_.end(); /* 可能删除 */) {
			rd_kafka_topic_partition_t* part = rd_kafka_topic_partition_list_find(
				offsets, rd_kafka_topic_name( i->first->rkt ), i->first->partition);
			if(part == nullptr) {
				std::printf("failed to detect commit offset\n");
				break;
			}
			if(part->offset >= i->first->offset) {
				i->second.co->next();
				i = self->commits_.erase(i);
			}else{
				++i;
			}
		}
	}
	void consumer_implement::commit(php::object& msg) {
		rd_kafka_message_t* mm = msg.native<message>()->msg_;
		commits_[mm] = commit_t {
			coroutine::current, consumer_, msg
		};
		rd_kafka_commit_message(kafka_, mm, true);
	}
	void consumer_implement::close_cb(uv_handle_t* handle) {
		consumer_implement* self = reinterpret_cast<consumer_implement*>(handle->data);
		delete self;
	}
	void consumer_implement::close() {
		rd_kafka_topic_partition_list_destroy(topic_);
		rd_kafka_conf_destroy(gconf_);
		rd_kafka_destroy(kafka_);
		if(context_ != nullptr) {
			kafka_request_t* ctx = reinterpret_cast<kafka_request_t*>(context_);
			ctx->co->next();
			delete ctx;
		}
		uv_check_stop(&check_);
		check_.data = this;
		uv_close((uv_handle_t*)&check_, close_cb);
	}
}
}
}
