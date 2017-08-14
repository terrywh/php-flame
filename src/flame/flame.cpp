#include "flame.h"
#include "fiber.h"

namespace flame {
	void init(php::extension_entry& ext) {
		zval* ff = static_cast<zval*>(async_);
		ZVAL_PTR(ff, &ext);
		loop = uv_default_loop();
		// 基础协程函数
		ext.add<go>("flame\\go");
		ext.add<run>("flame\\run");
		ext.add<fork>("flame\\fork");
	}
	php::value go(php::parameters& params) {
		php::callable& cb = params[0];
		php::value rv = fiber::start(cb);
		return rv;
	}
	php::value run(php::parameters& params) {
		signal(SIGPIPE, SIG_IGN); // 在 flame 运行中需要对 SIGPIPE 进行忽略
		uv_run(flame::loop, UV_RUN_DEFAULT);
		signal(SIGPIPE, SIG_DFL); // 恢复 SIGPIPE 默认值
		return nullptr;
	}
	php::value fork(php::parameters& params) {
		int pid = ::fork();
		if(pid == 0) {
			uv_loop_fork(flame::loop);
			return pid;
		}else{
			return 0;
		}
	}
}
