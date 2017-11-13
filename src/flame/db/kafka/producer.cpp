#include "../../coroutine.h"
#include "producer_implement.h"
#include "producer.h"


namespace flame {
namespace db {
namespace kafka {
	php::value producer::__construct(php::parameters& params) {
		impl = new producer_implement(this, params);
		return nullptr;
	}
	php::value producer::partitioner(php::parameters& params) {
		impl->partitioner(params[0]);
		return nullptr;
	}
	php::value producer::produce(php::parameters& params) {
		if(params[0].is_string() && params[1].is_string()) {
			impl->produce(params[0], params[1]);
		}else if(params[0].is_string()) {
			impl->produce(params[0]);
		}else{
			throw php::exception("failed to produce: illegal parameters");
		}
		return flame::async();
	}
	php::value producer::close(php::parameters& params) {
		if(impl != nullptr) {
			impl->close();
			impl = nullptr;
		}
		return nullptr;
	}
	php::value producer::__destruct(php::parameters& params) {
		if(impl != nullptr) impl->close();
		return nullptr;
	}
}
}
}
