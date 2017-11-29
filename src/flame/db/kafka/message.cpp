#include "message.h"
#include "consumer_implement.h"
#include "consumer.h"

namespace flame {
namespace db {
namespace kafka {
	void message::init(rd_kafka_message_t* m, consumer* c) {
		msg_ = m;
		prop("key", 3) = php::string((const char*)m->key, m->key_len);
		prop("val", 3) = php::string((const char*)m->payload, m->len);
		prop("time", 4) = rd_kafka_message_timestamp(m, NULL);
		ref_      = c;
		consumer_ = c;
	}
	// void message::init(rd_kafka_message_t* m, producer* p) {
	// 	msg_ = m;
	// 	prop("key",  3) = php::string((const char*)m->key, m->key_len);
	// 	prop("val",  3) = php::string((const char*)m->payload, m->len);
	// 	prop("time", 4) = rd_kafka_message_timestamp(m, NULL);
	// 	ref_      = p;
	// }
	php::value message::to_string(php::parameters& params) {
		return prop("val", 3);
	}
	php::value message::__destruct(php::parameters& params) {
		// 由于消息实际引用 kafka 的内部 buffer 故也须在内部线程中进行销毁
		consumer_request_t* ctx = new consumer_request_t {
			nullptr, consumer_->impl, ref_
		};
		ctx->msg.ptr(msg_);
		ctx->req.data = ctx;
		consumer_->impl->worker_.queue_work(&ctx->req,
			consumer_implement::destroy_msg_wk, consumer::default_cb);
		return nullptr;
	}
}
}
}
