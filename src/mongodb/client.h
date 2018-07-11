#pragma once

namespace flame {
namespace mongodb {
	class _connection_pool;
	class client: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params) { // 私有
			return nullptr;
		}
		php::value execute(php::parameters& params);
		php::value __get(php::parameters& params);
		php::value __isset(php::parameters& params);
		void execute(std::shared_ptr<coroutine> co, std::shared_ptr<bson_t> cmd);
	private:
		std::shared_ptr<_connection_pool> p_;
		friend php::value connect(php::parameters& params);
	};
}
}