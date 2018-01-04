#pragma once

namespace flame {
namespace db {
namespace rabbitmq {
	class client_implement;
	class consumer: public php::class_base {
	public:
		consumer();
		php::value __construct(php::parameters& params);
		php::value __destruct(php::parameters& params);
		php::value consume(php::parameters& params);
		php::value confirm(php::parameters& params);
		php::value reject(php::parameters& params);

		client_implement* impl;
	private:
		static void default_cb(uv_work_t* req, int status);
		static void consume_cb(uv_work_t* req, int status);

		amqp_boolean_t opt_no_local;
		amqp_boolean_t opt_no_ack;
		amqp_boolean_t opt_exclusive;
		table          php_arguments;
		amqp_table_t   opt_arguments;

		friend class client_implement;
		friend class message;
	};
}
}
}
