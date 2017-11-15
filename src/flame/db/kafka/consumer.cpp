#include "../../coroutine.h"
#include "consumer_implement.h"
#include "consumer.h"
#include "message.h"

namespace flame {
namespace db {
namespace kafka {
	php::value consumer::__construct(php::parameters& params) {
		impl = new consumer_implement(this, params);
		return nullptr;
	}
	php::value consumer::__destruct(php::parameters& params) {
		if(impl != nullptr) impl->close();
		return nullptr;
	}
	php::value consumer::consume(php::parameters& params) {
		if(params.length() > 0 && params[0].is_callable()) {
			impl->consume1(params[0]);
		}else{
			impl->consume2();
		}
		return flame::async();
	}
	php::value consumer::commit(php::parameters& params) {
		if(params.length() == 0 || !params[0].is_object()) {
			throw php::exception("failed to commit: illegal parameter");
		}
		php::object& obj = params[0];
		if(!obj.is_instance_of<message>()) {
			throw php::exception("failed to commit: illegal parameter");
		}
		impl->commit(obj);
		return flame::async();
	}
	php::value consumer::close(php::parameters& params) {
		if(impl != nullptr) {
			impl->close();
			impl = nullptr;
		}
		return nullptr;
	}
}
}
}
