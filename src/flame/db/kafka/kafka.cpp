#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "kafka.h"
#include "producer.h"
#include "consumer.h"
#include "message.h"

namespace flame {
namespace db {
namespace kafka {
	void init(php::extension_entry& ext) {
		php::class_entry<producer> class_producer("flame\\db\\kafka\\producer");
		class_producer.add<&producer::__construct>("__construct");
		class_producer.add<&producer::produce>("produce");
		class_producer.add<&producer::flush>("flush");
		class_producer.add<&producer::__destruct>("__destruct");
		ext.add(std::move(class_producer));

		php::class_entry<consumer> class_consumer("flame\\db\\kafka\\consumer");
		class_consumer.add<&consumer::__construct>("__construct");
		class_consumer.add<&consumer::consume>("consume");
		class_consumer.add<&consumer::commit>("commit");
		class_consumer.add<&consumer::__destruct>("__destruct");
		ext.add(std::move(class_consumer));

		php::class_entry<message> class_message("flame\\db\\kafka\\message");
		class_message.add(php::property_entry("key", std::string("")));
		class_message.add(php::property_entry("val", std::string("")));
		// class_message.add(php::property_entry("time", std::int64_t(0)));
		class_message.add<&message::to_string>("__toString");
		class_message.add<&message::timestamp>("timestamp");
		class_message.add<&message::timestamp_ms>("timestamp_ms");
		class_message.add<&message::__destruct>("__destruct");
		ext.add(std::move(class_message));
	}

	rd_kafka_conf_t* global_conf(php::array& gconf) {
		char   errstr[1024];
		size_t errlen = sizeof(errstr);
		rd_kafka_conf_t* conf = rd_kafka_conf_new();
		for(auto i=gconf.begin();i!=gconf.end();++i) {
			php::string& name = i->first.to_string();
			php::string& data = i->second.to_string();
			if(RD_KAFKA_CONF_OK != rd_kafka_conf_set(conf, name.c_str(), data.c_str(), errstr, errlen)) {
				throw php::exception("failed to set kafka global conf");
			}
		}
		return conf;
	}
}
}
}
