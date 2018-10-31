#pragma once

namespace flame {
namespace rabbitmq {
	class consumer: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params); // 私有
		php::value run(php::parameters& params);
		php::value confirm(php::parameters& params);
		php::value reject(php::parameters& params);
		php::value close(php::parameters& params);
	private:
		// 实际的客户端对象可能超过当前对象的生存期
		std::shared_ptr<client_context> amqp_;
		php::callable                     cb_;
		std::shared_ptr<coroutine>        co_;
		int                             flag_;
		std::string                    queue_;
		std::string                      tag_;
		
		friend php::value consume(php::parameters& params);
	};
}
}
