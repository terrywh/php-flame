#include "flame.h"
#include "fiber.h"
#include "process_manager.h"

extern char **environ;

namespace flame {
	uv_loop_t*     loop;
	php::value init_fx(php::parameters& params) {
		if(params.length() != 2 || !params[0].is_string() || !params[1].is_array()) {
			throw php::exception("illegal options for flame config");
		}
		std::string name = params[0];
		if(process_type == PROCESS_MASTER) {
			php::callable("cli_set_process_title").invoke(name + " (master)");
		}else{
			php::callable("cli_set_process_title").invoke(name + " (worker)");
		}
		// TODO call cli_set_process_title
		php::array& options = params[1];
		php::value& worker = options["worker"];
		if(!worker.is_long()) {
			worker = 0;
		}
		manager->init(worker.to_long());
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
		return fiber::start(static_cast<php::callable&>(params[0]));
	}
	php::value run(php::parameters& params) {
		manager->before_run_loop();
		int r = uv_run(flame::loop, UV_RUN_DEFAULT);
		manager->after_run_loop();
		// 标记退出状态用于确认是否自动重启工作进程
		if(process_type == PROCESS_WORKER) {
			exit(99);
		}
		return nullptr;
	}
	void init(php::extension_entry& ext) {
		zval* ff = static_cast<zval*>(fiber::async_);
		ZVAL_PTR(ff, &ext);
		loop = uv_default_loop();
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
