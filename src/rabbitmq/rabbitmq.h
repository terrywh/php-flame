#pragma once

namespace flame {
namespace rabbitmq {
	void declare(php::extension_entry& ext);
	php::value consume(php::parameters& params);
	php::value produce(php::parameters& params);

	php::array convert(const AMQP::Table& table);
	AMQP::Table convert(const php::array& table);
	class client_context {
	public:
		client_context(const php::string& addr);
		AMQP::LibBoostAsioHandler  handler;
		AMQP::TcpConnection     connection;
		AMQP::TcpChannel           channel;
	};
}
}