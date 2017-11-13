#pragma once

namespace flame {
namespace db {
namespace kafka {
	void init(php::extension_entry& ext);
	rd_kafka_conf_t* global_conf(php::array& gconf);
	rd_kafka_topic_conf_t* topic_conf(php::array& tconf);
}
}
}
