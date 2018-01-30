#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "producer.h"
#include "producer_implement.h"
#include "kafka.h"


namespace flame {
namespace db {
namespace kafka {
	producer_implement::producer_implement(producer* p, php::parameters& params)
	: producer_(p)
	, worker_()
	, kafka_(nullptr)
	, topic_(nullptr) {
		// !!! 简化处理，下面构建过程在主线程进行
		// ---------------------------------------------------------------------
		if(!params[0].is_array() || !params[1].is_array() || !params[2].is_string()) {
			throw php::exception("failed to create kafka producer: illegal parameters");
		}
		char   errstr[1024];
		size_t errlen = sizeof(errstr);

		rd_kafka_conf_t* gconf_ = global_conf(params[0]);
		rd_kafka_conf_set_error_cb(gconf_, error_cb);
		// rd_kafka_conf_set_dr_msg_cb(gconf_, dr_msg_cb);
		rd_kafka_conf_set_opaque(gconf_, this);
		kafka_ = rd_kafka_new(RD_KAFKA_PRODUCER, gconf_, errstr, errlen);
		if(kafka_ == nullptr) {
			throw php::exception(errstr);
		}
		// gconf_ = nullptr;

		rd_kafka_topic_conf_t* tconf_ = rd_kafka_topic_conf_new();
		php::array& tconf = params[1];
		for(auto i=tconf.begin();i!=tconf.end();++i) {
			php::string& name = i->first.to_string();
			if(name.length() == 14 && std::strncmp(name.c_str(), "partitioner_cb", 14) == 0) {
				partitioner_cb(tconf_, i->second);
			}else {
				php::string& data = i->second.to_string();
				if(RD_KAFKA_CONF_OK != rd_kafka_topic_conf_set(tconf_, name.c_str(), data.c_str(), errstr, errlen)) {
					throw php::exception("failed to set kafka conf");
				}
			}
		}

		php::string& name = params[2];
		topic_ =  rd_kafka_topic_new(kafka_, name.c_str(), tconf_);
		if(topic_ == nullptr) {
			rd_kafka_destroy(kafka_);
			throw php::exception("failed to create kafka topic");
		}
		// tconf_ = nullptr;
	}
	int32_t producer_implement::partitioner_cb (
			const rd_kafka_topic_t *rkt, const void *keydata, size_t keylen,
			int32_t partition_cnt, void *rkt_opaque, void *msg_opaque) {

		producer_implement* self = reinterpret_cast<producer_implement*>(msg_opaque);
		php::string key((const char*)keydata, keylen);
		return self->partitioner_fn.invoke(key, partition_cnt);
	}
	void producer_implement::partitioner_cb(rd_kafka_topic_conf_t* tconf_, php::value& cb) {
		if(cb.is_string()) {
			php::string& str = cb;
			if(str.length() == 10 && std::strncmp(str.c_str(), "consistent", 10) == 0) {
				rd_kafka_topic_conf_set_partitioner_cb(tconf_, rd_kafka_msg_partitioner_consistent);
			}else if(str.length() == 17 && std::strncmp(str.c_str(), "consistent_random", 17) == 0) {
				rd_kafka_topic_conf_set_partitioner_cb(tconf_, rd_kafka_msg_partitioner_consistent_random);
			}else if(str.length() == 6 && std::strncmp(str.c_str(), "random", 6) == 0) {
				rd_kafka_topic_conf_set_partitioner_cb(tconf_, rd_kafka_msg_partitioner_random);
			}else{
				throw php::exception("failed to set partitioner_cb: unkonwn partitioner");
			}
		}else if(cb.is_callable()) {
			partitioner_fn = cb;
			rd_kafka_topic_conf_set_partitioner_cb(tconf_, partitioner_cb);
		}else{
			throw php::exception("failed to set partitioner_cb: illegal partitioner");
		}
	}
	void producer_implement::error_cb(rd_kafka_t *rk, int err, const char *reason, void *opaque) {
		producer_implement* self = reinterpret_cast<producer_implement*>(opaque);
		php::info("kafka error: (%d) %s", err, reason);
	}
	void producer_implement::produce_wk(uv_work_t* handle) {
		producer_request_t* ctx = reinterpret_cast<producer_request_t*>(handle->data);
		
		if(ctx->key.is_string()) {
			rd_kafka_produce(ctx->self->topic_, RD_KAFKA_PARTITION_UA, RD_KAFKA_MSG_F_COPY,
				ctx->val.data(), ctx->val.length(), ctx->key.c_str(), ctx->key.length(), ctx);
		}else{
			rd_kafka_produce(ctx->self->topic_, RD_KAFKA_PARTITION_UA, RD_KAFKA_MSG_F_COPY,
				ctx->val.data(), ctx->val.length(), nullptr, 0, ctx);
		}
		// while(ctx->rv.is_object()) {
		// 	rd_kafka_poll(ctx->self->kafka_, 1000);
		// }
		rd_kafka_poll(ctx->self->kafka_, 0);
		ctx->rv = true;
	}
	// void producer_implement::dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t * rkmessage, void *opaque) {
	// 	producer_request_t* ctx = reinterpret_cast<producer_request_t*>(rkmessage->_private);
	// 	// TODO 错误、失败处理
	// 	ctx->rv = php::BOOL_YES;
	// }
	void producer_implement::flush_wk(uv_work_t* req) {
		producer_request_t* ctx = reinterpret_cast<producer_request_t*>(req->data);
		rd_kafka_flush(ctx->self->kafka_, 10000);
		rd_kafka_poll(ctx->self->kafka_, 0);
	}
	void producer_implement::destroy_wk(uv_work_t* req) {
		producer_implement* self = reinterpret_cast<producer_implement*>(req->data);
		rd_kafka_flush(self->kafka_, 10000);
		rd_kafka_poll(self->kafka_, 0);
		rd_kafka_topic_destroy(self->topic_);
		rd_kafka_destroy(self->kafka_);
	}
	void producer_implement::destroy_cb(uv_work_t* req, int status) {
		delete reinterpret_cast<producer_implement*>(req->data);
	}
}
}
}
