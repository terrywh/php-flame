#pragma once

namespace flame {
namespace db {
namespace kafka {
	class producer_implement;
	class producer: public php::class_base {
	public:
		php::value __construct(php::parameters& params);
		php::value __destruct(php::parameters& params);
		php::value produce(php::parameters& params);
		php::value flush(php::parameters& params);
		producer_implement* impl;
	private:
		static void default_cb(uv_work_t* req, int status);
	};

}
}
}
