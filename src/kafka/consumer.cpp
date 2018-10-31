#include "../coroutine.h"
#include "kafka.h"
#include "consumer.h"

namespace flame {
namespace kafka {

    void consumer::declare(php::extension_entry& ext) {
		php::class_entry<consumer> class_consumer("flame\\kafka\\consumer");
		class_consumer
			.method<&consumer::__construct>("__construct", {}, php::PRIVATE)
			.method<&consumer::run>("run", {
				{"callable", php::TYPE::CALLABLE},
			})
			.method<&consumer::close>("close");
		ext.add(std::move(class_consumer));
	}

	php::value consumer::__construct(php::parameters& params) {
		return nullptr;
	}
	php::value consumer::run(php::parameters& params) {
		return nullptr;
	}
	php::value consumer::close(php::parameters& params) {
		return nullptr;
	}
}
}