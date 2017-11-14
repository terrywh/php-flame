#include "producer.h"
#include "producer_implement.h"
#include "kafka.h"
#include "../../coroutine.h"

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
		rd_kafka_conf_set_dr_msg_cb(gconf_, dr_msg_cb);
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
		php::fail("kafka error: (%d) %s", err, reason);
	}
	typedef struct kafka_request_t {
		coroutine*            co;
		producer_implement* self;
		php::value           ref;
		php::string          key;
		php::string          val;
		php::value            rv;
		uv_work_t            req;
	} kafka_request_t;
	void producer_implement::produce_wk(uv_work_t* handle) {
		kafka_request_t* ctx = reinterpret_cast<kafka_request_t*>(handle->data);

		if(ctx->key.is_string()) {
			rd_kafka_produce(ctx->self->topic_, RD_KAFKA_PARTITION_UA, 0,
				ctx->val.data(), ctx->val.length(), ctx->key.c_str(), ctx->key.length(), ctx);
		}else{
			rd_kafka_produce(ctx->self->topic_, RD_KAFKA_PARTITION_UA, 0,
				ctx->val.data(), ctx->val.length(), nullptr, 0, ctx);
		}
		do {
			rd_kafka_poll(ctx->self->kafka_, 10);
		}while(ctx->rv.is_undefined());
	}
	void producer_implement::dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t * rkmessage, void *opaque) {
		kafka_request_t* ctx = reinterpret_cast<kafka_request_t*>(rkmessage->_private);
		// TODO 错误、失败处理
		ctx->rv = bool(true);
	}
	void producer_implement::produce_cb(uv_work_t* handle, int status) {
		kafka_request_t* ctx = reinterpret_cast<kafka_request_t*>(handle->data);
		ctx->co->next(ctx->rv);
		delete ctx;
	}
	void producer_implement::produce(const php::string& val, const php::string& key) {
		kafka_request_t* ctx = new kafka_request_t {
			coroutine::current, this, producer_, key, val
		};
		ctx->req.data = ctx;
		worker_.queue_work(&ctx->req, produce_wk, produce_cb);
	}
	void producer_implement::close() {
		// !!! 简化处理，下面清理过程在主线程进行
		rd_kafka_flush(kafka_, 1000);

		rd_kafka_topic_destroy(topic_);
		rd_kafka_destroy(kafka_);
	}
}
}
}
