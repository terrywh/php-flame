#pragma once

namespace flame {
namespace db {
namespace rabbitmq {
	class client_implement;
	class producer: public php::class_base {
	public:
		producer();
		php::value __construct(php::parameters& params);
		php::value __destruct(php::parameters& params);
		php::value produce(php::parameters& params);
		php::value flush(php::parameters& params);
		client_implement* impl;
	private:

		php::string    php_exchange;
		amqp_bytes_t   opt_exchange; // 引用上述 php_exchange 字符串内存
		amqp_boolean_t opt_mandatory;
		amqp_boolean_t opt_immediate;

		friend class client_implement;
	};
}
}
}
