#include "flame.h"
#include "promise.h"
#include "asyncfn.h"

namespace flame {
	void init(php::extension_entry& ext) {
		php::class_entry<promise::future_t> class_future_promise("flame\\future_promise");
		php::class_entry<asyncfn::future_t> class_future_asyncfn("flame\\future_asyncfn");
		ext.add(std::move(class_future_promise));
		ext.add(std::move(class_future_asyncfn));
		ext.add<go>("flame\\go");
		ext.add<run>("flame\\run");
		ext.add<async>("flame\\async");
	}
	void coroutine(php::generator gen) {
	NEXT:
		if(EG(exception) || !gen.valid()) {
			return;
		}
		php::value v = gen.current();
		if(!v.is_object()) {
			gen.send(v);
			goto NEXT;
		}
		php::object o = v;
		if(o.is_instance_of<promise::future_t>()) {
			promise::future_t* s = o.native<promise::future_t>();
			s->t->set_continue(gen);
			return;
		}
		if(o.is_instance_of<asyncfn::future_t>()) {
			asyncfn::future_t* s = o.native<asyncfn::future_t>();
			s->t->set_continue(gen);
			return;
		}
		gen.send(v);
		goto NEXT;
	}

	php::value go(php::parameters& params) {
		php::value& obj = params[0];
	IS_GENERATOR:
		if(obj.is_generator()) {
			coroutine(obj);
		}else if(obj.is_callable()) {
			obj = static_cast<php::callable&>(obj).invoke();
			goto IS_GENERATOR;
		}else{
			throw php::exception(std::string("not a instance of Generator"));
		}
		return nullptr;
	}

	php::value run(php::parameters& params) {
		uv_run(uv_default_loop(), UV_RUN_DEFAULT);
		return nullptr;
	}
}

