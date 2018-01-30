#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "table.h"
#include "client_implement.h"
#include "consumer.h"
#include "message.h"


namespace flame {
namespace db {
namespace rabbitmq {
	void message::init(amqp_envelope_t* e, consumer* c) {
		envelope_ = e;
		prop("exchange", 8) = php::string((const char*)e->exchange.bytes, e->exchange.len);
		prop("redelivered", 11) = e->redelivered ? php::BOOL_YES : php::BOOL_NO;
		prop("key", 3) = php::string((const char*)e->routing_key.bytes, e->routing_key.len);
		prop("val", 3) = php::string((const char*)e->message.body.bytes, e->message.body.len);
		init_property(&e->message.properties);
		ref_      = c;
		consumer_ = c;
	}
	void message::init_property(amqp_basic_properties_t *p) {
		prop("content_type", 12)     = php::string((const char*)p->content_type.bytes, p->content_type.len);
		prop("content_encoding", 16) = php::string((const char*)p->content_encoding.bytes, p->content_encoding.len);
		prop("headers", 7)           = table::convert(&p->headers);
		prop("delivery_mode", 13)    = int64_t(p->delivery_mode);
		prop("priority", 8)          = int64_t(p->priority);
		prop("correlation_id", 14)   = php::string((const char*)p->correlation_id.bytes, p->correlation_id.len);
		prop("reply_to", 8)          = php::string((const char*)p->reply_to.bytes, p->reply_to.len);
		prop("expiration", 10)       = php::string((const char*)p->expiration.bytes, p->expiration.len);
		prop("message_id", 10)       = php::string((const char*)p->message_id.bytes, p->message_id.len);
		ts_                          = int64_t(p->timestamp); // AMQP 0-9-1 规定 timestamp 为秒级时间戳
		prop("type", 4)              = php::string((const char*)p->type.bytes, p->type.len);
		prop("user_id", 7)           = php::string((const char*)p->user_id.bytes, p->user_id.len);
		prop("app_id", 6)            = php::string((const char*)p->app_id.bytes, p->app_id.len);
		prop("cluster_id", 10)       = php::string((const char*)p->cluster_id.bytes, p->cluster_id.len);
	}
	php::value message::to_string(php::parameters& params) {
		return prop("val", 3);
	}
	php::value message::timestamp_ms(php::parameters& params) {
		return ts_ * 1000;
	}
	php::value message::timestamp(php::parameters& params) {
		return ts_;
	}
	php::value message::__destruct(php::parameters& params) {
		// 由于消息实际引用 kafka 的内部 buffer 故也须在内部线程中进行销毁
		client_request_t* ctx = new client_request_t {
			nullptr, consumer_->impl, php::string(nullptr), php::string(nullptr), ref_
		};
		ctx->msg.ptr(envelope_);
		ctx->req.data = ctx;
		consumer_->impl->worker_.queue_work(&ctx->req,
			client_implement::destroy_envelope_wk, destroy_envelope_cb);
		return nullptr;
	}
	void message::destroy_envelope_cb(uv_work_t* req, int status) {
		delete reinterpret_cast<client_request_t*>(req->data);
	}
	
}
}
}
