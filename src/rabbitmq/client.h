#pragma once

namespace flame {
namespace rabbitmq {
	class client: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params); // 私有
		// 创建 consumer
		php::value consume(php::parameters& params);
		// 创建 producer
		php::value produce(php::parameters& params);
	private:
		// 实际的客户端对象可能超过当前对象的生存期
		context_type amqp_;
		friend php::value connect(php::parameters& params);
	};
}
}