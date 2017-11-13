#include "kafka.h"

namespace flame {
namespace db {
namespace kafka {
	void init(php::extension_entry& ext) {

	}

	rd_kafka_conf_t* gconf(php::array& gconf) {
		char   errstr[1024];
		size_t errlen = sizeof(errstr);
		rd_kafka_conf_t* conf = rd_kafka_conf_new();
		for(auto i=gconf.begin();i!=gconf.end();++i) {
			php::string& name = i->first.to_string();
			php::string& data = i->second.to_string();
			if(RD_KAFKA_CONF_OK != rd_kafka_conf_set(conf, name.c_str(), data.c_str(), errstr, errlen)) {
				throw php::exception("failed to set kafka conf");
			}
		}
		return conf;
	}

	rd_kafka_topic_conf_t* topic_conf(php::array& tconf) {
		char   errstr[1024];
		size_t errlen = sizeof(errstr);
		rd_kafka_topic_conf_t* conf = rd_kafka_topic_conf_new();
		for(auto i=tconf.begin();i!=tconf.end();++i) {
			php::string& name = i->first.to_string();
			php::string& data = i->second.to_string();
			if(RD_KAFKA_CONF_OK != rd_kafka_topic_conf_set(conf, name.c_str(), data.c_str(), errstr, errlen)) {
				throw php::exception("failed to set kafka conf");
			}
		}
		return conf;
	}
}
}
}
