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
	void message::init(consumer* c) {
		prop("exchange", 8) = php::string((const char*)envelope_.exchange.bytes, envelope_.exchange.len);
		prop("redelivered", 11) = envelope_.redelivered ? php::BOOL_TRUE : php::BOOL_NO;
		prop("key", 3) = php::string((const char*)envelope_.routing_key.bytes, envelope_.routing_key.len);
		prop("val", 3) = php::string((const char*)envelope_.message.body.bytes, envelope_.message.body.len);
		init_property(&envelope_.message.properties);
		ref_      = php::object(c);
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
}
}
}
