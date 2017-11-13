#pragma once

namespace flame {
namespace db {
namespace kafka {
	class producer_implement;
	class producer: public php::class_base {
	public:
		php::value __construct(php::parameters& params);
		php::value partitioner(php::parameters& params);
		php::value produce(php::parameters& params);
		php::value close(php::parameters& params);
		php::value __destruct(php::parameters& params);
		producer_implement* impl = nullptr;
	};

}
}
}
