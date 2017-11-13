#pragma once

namespace flame {
namespace db {
namespace kafka {
	class consumer_implement;
	class consumer: public php::class_base {
	public:
		php::value __construct(php::parameters& params);
		php::value consume(php::parameters& params);
		php::value commit(php::parameters& params);
		php::value close(php::parameters& params);
		php::value __destruct(php::parameters& params);
		consumer_implement* impl;
	};
}
}
}
