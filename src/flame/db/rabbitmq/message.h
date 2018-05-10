#pragma once

namespace flame {
namespace db {
namespace rabbitmq {
	
	class consumer;
	class message: public php::class_base {
	public:
		void init(consumer* c);
		void init_property(amqp_basic_properties_t *p);
		php::value __construct(php::parameters& params) {
			return nullptr;
		}
		php::value to_string(php::parameters& params);
		php::value timestamp_ms(php::parameters& params);
		php::value timestamp(php::parameters& params);
	private:
		amqp_envelope_t     envelope_;
		php::object         ref_;
		consumer*           consumer_;
		int64_t             ts_;

		static void destroy_message_cb(uv_work_t* req, int status);
		
		friend class client_implement;
	};
}
}
}
