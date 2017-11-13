#pragma once

namespace flame {
namespace db {
namespace kafka {
	class consumer;
	class producer;
	class message: public php::class_base {
	public:
		void init(rd_kafka_message_t* message, consumer* c);
		void init(rd_kafka_message_t* message, producer* p);
		php::value to_string(php::parameters& params);
	private:
		rd_kafka_message_t* msg_;
		php::object         ref_;
		friend class consumer_implement;
	};
}
}
}
