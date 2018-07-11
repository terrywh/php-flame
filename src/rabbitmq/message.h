#pragma once

namespace flame {
namespace rabbitmq {
	class message: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params); // 私有
	private:
		void build_ex(const AMQP::Message& msg, std::uint64_t tag);
		void build_ex(AMQP::Envelope& env);
		std::uint64_t tag_;
		friend class consumer;
		friend class producer;
	};
}
}
