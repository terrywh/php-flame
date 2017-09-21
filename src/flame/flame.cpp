#include "flame.h"
#include "fiber.h"
#include "process_manager.h"

extern char **environ;

namespace flame {
	uv_loop_t*  loop;
	static bool initialized = false;
	static void init_threadpool_size() {
		// 设置环境变量
		char   edata[8];
		size_t esize = sizeof(edata);
		int r = uv_os_getenv("UV_THREADPOOL_SIZE", edata, &esize);
		uv_os_setenv("UV_THREADPOOL_SIZE", "1");
		queue(nullptr, nullptr, nullptr); // libuv 实现首次 queue 时初始化线程
		// 还原环境变量
		if(!r) {
			uv_os_setenv("UV_THREADPOOL_SIZE", edata);
		}else{
			uv_os_unsetenv("UV_THREADPOOL_SIZE");
		}
	}
	static void init_process_name(const std::string& name) {
		if(process_type == PROCESS_MASTER) {
			php::callable("cli_set_process_title").invoke(name + " (master)");
		}else{
			php::callable("cli_set_process_title").invoke(name + " (worker)");
		}
	}
	php::value init_fx(php::parameters& params) {
		if(params.length() > 0 && params[0].is_string()) {
			init_process_name(params[0]);
		}
		if(params.length() > 1 && params[1].is_array()) {
			php::array& options = params[1];
			php::value& worker = options["worker"];
			if(!worker.is_long()) {
				worker = 0;
			}
			manager->init(worker.to_long());
		}else{
			manager->init(0);
		}
		// 设置环境变量改变线程池大小，生效后还原
		init_threadpool_size();
		initialized = true;
		return nullptr;
	}
	php::value on_quit(php::parameters& params) {
		if(params.length() > 0 && params[0].is_callable()) {
			manager->push_exit_cb(params[0]);
		}else{
			throw php::exception("illegal parameter: callable is required");
		}
		return nullptr;
	}
	php::value go(php::parameters& params) {
		if(!initialized) init_fx(params);
		return fiber::start(static_cast<php::callable&>(params[0]));
	}
	php::value run(php::parameters& params) {
		if(params.length() > 0) {
			throw php::exception("'flame\\run()' must be called without any parameters");
		}
		if(!initialized) init_fx(params);
		manager->before_run_loop();
		int r = uv_run(flame::loop, UV_RUN_DEFAULT);
		manager->after_run_loop();
		delete flame::loop;
		flame::loop = nullptr;
		// 标记退出状态用于确认是否自动重启工作进程
		if(process_type == PROCESS_WORKER) {
			exit(99);
		}
		return nullptr;
	}
	void init(php::extension_entry& ext) {
		zval* ff = static_cast<zval*>(fiber::async_);
		ZVAL_PTR(ff, &ext);
		loop = new uv_loop_t;
		uv_loop_init(loop);
		// 基础函数
		ext.add<init_fx>("flame\\init");
		ext.add<on_quit>("flame\\on_quit");
		ext.add<go>("flame\\go");
		ext.add<run>("flame\\run");
		// 确认当前是父进程还是子进程
		char   buffer[8];
		size_t buffer_size = 8;
		if(uv_os_getenv("FLAME_CLUSTER_WORKER", buffer, &buffer_size) < 0) {
			process_type = PROCESS_MASTER;
			manager = new process_manager();
		}else{
			process_type = PROCESS_WORKER;
		}
	}

}
