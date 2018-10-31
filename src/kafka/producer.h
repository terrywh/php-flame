#pragma once

namespace flame {
namespace kafka {
	class producer: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params); // 私有
		php::value publish(php::parameters& params);
	private:
		friend class client;
	};
}
}
