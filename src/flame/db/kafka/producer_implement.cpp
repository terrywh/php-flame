#include "producer.h"
#include "producer_implement.h"
#include "kafka.h"
#include "../../coroutine.h"

namespace flame {
namespace db {
namespace kafka {
	producer_implement::producer_implement(producer* p, php::parameters& params)
	:producer_(p) {
		if(!params[0].is_array() || !params[1].is_array() || !params[2].is_string()) {
			throw php::exception("failed to create kafka producer: illegal parameters");
		}
		char   errstr[1024];
		size_t errlen = sizeof(errstr);

		gconf_ = global_conf(params[0]);
		rd_kafka_conf_set_error_cb(gconf_, error_cb);
		rd_kafka_conf_set_dr_msg_cb(gconf_, produce_cb);
		rd_kafka_conf_set_opaque(gconf_, this);
		tconf_ = topic_conf(params[1]);
		kafka_ = rd_kafka_new(RD_KAFKA_PRODUCER, gconf_, errstr, errlen);
		if(kafka_ == nullptr) {
			throw php::exception(errstr);
		}
		php::string& name = params[2];
		topic_ = rd_kafka_topic_new(kafka_, name.c_str(), tconf_);
		if(topic_ == nullptr) {
			throw php::exception(errstr);
		}
		uv_check_init(flame::loop, &check_);
	}
	int32_t producer_implement::partitioner_cb (
			const rd_kafka_topic_t *rkt, const void *keydata, size_t keylen,
			int32_t partition_cnt, void *rkt_opaque, void *msg_opaque) {

		producer_implement* self = reinterpret_cast<producer_implement*>(msg_opaque);
		php::string key((const char*)keydata, keylen);
		return self->partitioner_fn.invoke(key, partition_cnt);
	}
	void producer_implement::partitioner(php::value& cb) {
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
		php::object          ref;
		php::value          data;
	} kafka_request_t;
	void producer_implement::produce_cb(uv_check_t* handle) {
		kafka_request_t* ctx = reinterpret_cast<kafka_request_t*>(handle->data);
		rd_kafka_poll(ctx->self->kafka_, 0);
	}
	void producer_implement::produce_cb(rd_kafka_t *rk, const rd_kafka_message_t * rkmessage, void *opaque) {
		kafka_request_t* ctx = reinterpret_cast<kafka_request_t*>(opaque);
		// TODO 错误、失败处理
		ctx->co->next(bool(true));
		uv_check_stop(&ctx->self->check_);
		delete ctx;
	}
	void producer_implement::produce(php::string& payload, php::string& key) {
		kafka_request_t* ctx = new kafka_request_t {
			coroutine::current, this, producer_, payload
		};
		rd_kafka_produce(topic_, RD_KAFKA_PARTITION_UA, 0,
			payload.data(), payload.length(), key.c_str(), key.length(), ctx);

		check_.data = ctx;
		uv_check_start(&check_, produce_cb);
	}
	void producer_implement::produce(php::string& payload) {
		kafka_request_t* ctx = new kafka_request_t {
			coroutine::current, this, producer_, payload
		};
		rd_kafka_produce(topic_, RD_KAFKA_PARTITION_UA, 0,
			payload.data(), payload.length(), nullptr, 0, ctx);

		check_.data = ctx;
		uv_check_start(&check_, produce_cb);
	}
	void producer_implement::close_cb(uv_handle_t* handle) {
		producer_implement* self = reinterpret_cast<producer_implement*>(handle->data);
		delete self;
	}
	void producer_implement::close() {
		rd_kafka_topic_conf_destroy(tconf_);
		rd_kafka_conf_destroy(gconf_);
		rd_kafka_topic_destroy(topic_);
		rd_kafka_destroy(kafka_);
		uv_check_stop(&check_);
		check_.data = this;
		uv_close((uv_handle_t*)&check_, close_cb);
	}
}
}
}
