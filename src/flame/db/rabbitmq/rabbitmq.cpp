#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "rabbitmq.h"
#include "table.h"
#include "producer.h"
#include "consumer.h"
#include "message.h"

namespace flame {
namespace db {
namespace rabbitmq {
	void init(php::extension_entry& ext) {
		php::class_entry<producer> class_producer("flame\\db\\rabbitmq\\producer");
		class_producer.add<&producer::__construct>("__construct");
		class_producer.add<&producer::__destruct>("__destruct");
		class_producer.add<&producer::produce>("produce");
		class_producer.add<&producer::flush>("flush");
		ext.add(std::move(class_producer));

		php::class_entry<consumer> class_consumer("flame\\db\\rabbitmq\\consumer");
		class_consumer.add<&consumer::__construct>("__construct");
		class_consumer.add<&consumer::__destruct>("__destruct");
		class_consumer.add<&consumer::consume>("consume");
		class_consumer.add<&consumer::confirm>("confirm");
		class_consumer.add<&consumer::reject>("reject");
		ext.add(std::move(class_consumer));

		php::class_entry<message> class_message("flame\\db\\rabbitmq\\message");
		class_message.prop({"exchange", std::string("")});
		class_message.prop({"redelivered", php::BOOL_NO});
		class_message.prop({"key", std::string("")});
		class_message.prop({"val", std::string("")});

		class_message.prop({"content_type", std::string("")});
		class_message.prop({"content_encoding", std::string("")});
		class_message.prop({"headers", nullptr});
		class_message.prop({"delivery_mode", 0});
		class_message.prop({"priority", 0});
		class_message.prop({"correlation_id", std::string("")});
		class_message.prop({"reply_to", std::string("")});
		class_message.prop({"expiration", std::string("")});
		class_message.prop({"message_id", std::string("")});
		class_message.prop({"type", std::string("")});
		class_message.prop({"user_id", std::string("")});
		class_message.prop({"app_id", std::string("")});
		class_message.prop({"cluster_id", std::string("")});

		class_message.add<&message::__construct>("__construct", ZEND_ACC_PRIVATE); // 私有
		class_message.add<&message::timestamp>("timestamp");
		class_message.add<&message::timestamp_ms>("timestamp_ms");
		class_message.add<&message::to_string>("__toString");
		ext.add(std::move(class_message));
	}
}
}
}
