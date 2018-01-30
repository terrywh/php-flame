#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "kafka.h"
#include "consumer.h"
#include "consumer_implement.h"
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
		// 全局配置
		rd_kafka_conf_t* gconf_ = global_conf(params[0]);
		size_t len;
		if(rd_kafka_conf_get(gconf_, "group.id", nullptr, &len) != RD_KAFKA_CONF_OK) {
			rd_kafka_conf_destroy(gconf_);
			throw php::exception("failed to create kafka consumer: global option 'group.id' must be sepecified");
		}
		rd_kafka_conf_set_error_cb(gconf_, error_cb);
		rd_kafka_conf_set_offset_commit_cb(gconf_, offset_commit_cb);
		rd_kafka_conf_set_opaque(gconf_, this);
		// 话题配置
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
		// KAFKA 对象
		kafka_ = rd_kafka_new(RD_KAFKA_CONSUMER, gconf_, errstr, errlen);
		if(kafka_ == nullptr) {
			rd_kafka_conf_destroy(gconf_);
			throw php::exception(errstr);
		}
		if(params[2].is_array()) {
			php::array& topics = params[2];
			topic_ = rd_kafka_topic_partition_list_new(topics.length());
			for(auto i=topics.begin(); i!=topics.end(); ++i) {
				php::string& topic = i->second.to_string();
				rd_kafka_topic_partition_list_add(topic_, topic.c_str(), RD_KAFKA_PARTITION_UA);
			}
		}else{
			php::string topic = params[2].to_string();
			topic_ = rd_kafka_topic_partition_list_new(1);
			rd_kafka_topic_partition_list_add(topic_, topic.c_str(), RD_KAFKA_PARTITION_UA);
		}
		// 订阅开始消费
		rd_kafka_resp_err_t error = rd_kafka_subscribe(kafka_, topic_);
		if(error != RD_KAFKA_RESP_ERR_NO_ERROR) {
			throw php::exception(rd_kafka_err2str(error), error);
		}
	}
	void consumer_implement::error_cb(rd_kafka_t *rk, int err, const char *reason, void *opaque) {
		consumer_implement* self = reinterpret_cast<consumer_implement*>(opaque);
		php::info("kafka error: (%d) %s", err, reason);
	}
	void consumer_implement::consume_wk(uv_work_t* handle) {
		consumer_request_t* ctx = reinterpret_cast<consumer_request_t*>(handle->data);
		rd_kafka_message_t* msg = nullptr;
		int64_t timeout = ctx->rv, elapse = 0;
		while(true) {
			msg     = rd_kafka_consumer_poll(ctx->self->kafka_, 100);
			elapse += 100;
			if(msg) {
				if(msg->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
					rd_kafka_message_destroy(msg);
					// 继续等待数据
				}else{
					ctx->msg.ptr(msg);
					// 返回的数据可能包含异常情况需要处理
					break;
				}
			}else if(timeout > 0 && elapse > timeout) { // 若用户指定了超时，进行超时处理
				ctx->msg = nullptr;
				break;
			}
		}
		while (rd_kafka_outq_len(ctx->self->kafka_) > 0) {
			rd_kafka_poll(ctx->self->kafka_, 1000);
		}
	}
	void consumer_implement::commit_wk(uv_work_t* handle) {
		consumer_request_t* ctx = reinterpret_cast<consumer_request_t*>(handle->data);
		ctx->self->ctx_ = ctx; // 用于在 offset_commit_cb 中访问当前 context 上下问
		rd_kafka_commit_message(ctx->self->kafka_, ctx->msg.native<message>()->msg_, true);
		while(ctx->rv.is_object()) {
			rd_kafka_poll(ctx->self->kafka_, 1000);
		}
	}
	void consumer_implement::offset_commit_cb(rd_kafka_t *rk, rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t *offsets, void *opaque) {
		// TODO 如何确认得到的 offset commit 已包含了上述 message ？
		consumer_implement* self = reinterpret_cast<consumer_implement*>(opaque);
		if(self->ctx_) {
			self->ctx_->rv = php::BOOL_YES;
			self->ctx_ = nullptr;
		}
	}
	void consumer_implement::destroy_wk(uv_work_t* handle) {
		consumer_implement* self = reinterpret_cast<consumer_implement*>(handle->data);
	
		rd_kafka_consumer_close(self->kafka_);
		while (rd_kafka_outq_len(self->kafka_) > 0) {
			rd_kafka_poll(self->kafka_, 1000);
		}
		rd_kafka_topic_partition_list_destroy(self->topic_);
		rd_kafka_destroy(self->kafka_);
	}
	void consumer_implement::destroy_cb(uv_work_t* handle, int status) {
		delete reinterpret_cast<consumer_implement*>(handle->data);
	}
	void consumer_implement::destroy_msg_wk(uv_work_t* handle) {
		consumer_request_t* ctx = reinterpret_cast<consumer_request_t*>(handle->data);
		rd_kafka_message_t* msg = ctx->msg.ptr<rd_kafka_message_t>();
		
		rd_kafka_message_destroy(msg);
	}
}
}
}
