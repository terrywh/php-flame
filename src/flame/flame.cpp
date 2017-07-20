#include "flame.h"
#include "fiber.h"

namespace flame {
	void init(php::extension_entry& ext) {
		zval* ff = static_cast<zval*>(async);
		ZVAL_PTR(ff, &ext);
		loop = uv_default_loop();
		ext.add<go>("flame\\go");
		ext.add<run>("flame\\run");
	}
	
	php::value go(php::parameters& params) {
		php::callable& cb = params[0];
		if(!fiber::start(cb)) {
			throw php::exception("failed to launch fiber: not a generator or error occurred");
		}
		return nullptr;
	}

	php::value run(php::parameters& params) {
		signal(SIGPIPE, SIG_IGN); // 在 flame 运行中需要对 SIGPIPE 进行忽略
		uv_run(flame::loop, UV_RUN_DEFAULT);
		signal(SIGPIPE, SIG_DFL); // 恢复 SIGPIPE 默认值
		return nullptr;
	}
}

