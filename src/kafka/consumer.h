#pragma once

namespace flame {
namespace kafka {
	class consumer: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params); // 私有
		php::value run(php::parameters& params);
		php::value close(php::parameters& params);
	private:
		php::callable              cb_;
		std::shared_ptr<coroutine> co_;
		friend class client;
	};
}
}
