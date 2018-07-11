#pragma once

namespace flame {
namespace rabbitmq {
	void declare(php::extension_entry& ext);
	php::value connect(php::parameters& params);
	php::array convert(const AMQP::Table& table);
	AMQP::Table convert(const php::array& table);
	struct context_type {
		std::shared_ptr<AMQP::LibBoostAsioHandler> handler;
		std::shared_ptr<AMQP::TcpConnection>    connection;
		std::shared_ptr<AMQP::TcpChannel>          channel;
	};
}
}