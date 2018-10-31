#pragma once

namespace flame {
namespace kafka {
	class message: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params); // 私有
		php::value to_json(php::parameters& params);
		php::value to_string(php::parameters& params);
	private:
        
	};
}
}
