#include "message.h"

namespace flame {
namespace db {
namespace kafka {
	void message::init(rd_kafka_message_t* m, consumer* c) {
		msg_ = m;
		prop("key", 3) = php::string((const char*)m->key, m->key_len);
		prop("val", 3) = php::string((const char*)m->payload, m->len);
		prop("time", 4) = rd_kafka_message_timestamp(m, NULL);
		ref_      = c;
	}
	void message::init(rd_kafka_message_t* m, producer* p) {
		msg_ = m;
		prop("key",  3) = php::string((const char*)m->key, m->key_len);
		prop("val",  3) = php::string((const char*)m->payload, m->len);
		prop("time", 4) = rd_kafka_message_timestamp(m, NULL);
		ref_      = p;
	}
	php::value message::to_string(php::parameters& params) {
		return prop("val", 3);
	}
	php::value message::__destruct(php::parameters& params) {
		rd_kafka_message_destroy(msg_);
		return nullptr;
	}
}
}
}
