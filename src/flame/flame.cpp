#include "coroutine.h"
#include "process.h"
#include "log/log.h"

namespace flame {
	static int status = 0;

	static void init_opts(php::array& opts) {
		php::value worker_count = opts.at("worker", 6);
		if(worker_count.is_long()) {
			process_count = static_cast<int>(worker_count);
		}else{
			process_count = 0; // 不启动工作进程
		}
	}

	void free_handle_cb(uv_handle_t* handle) {
		free(handle);
	}
	void free_data_cb(uv_handle_t* handle) {
		free(handle->data);
	}
	static php::value init(php::parameters& params) {
		// 应用名用于设置进程名称
		process_name = static_cast<std::string>(params[0]);
		// 进程数量
		if(params.length() > 1) {
			init_opts(params[1]);
		}
		process_self->init();
		// 直接在 module_startup 中进行 rotate 会改变无参时 PHP 命令的行为（直接退出）
		log::default_logger->init();
		status |= 0x01;
		return nullptr;
	}
	static php::value go(php::parameters& params) {
		if((status & 0x01) < 0x01) throw php::exception("flame not yet initialized");
		status |= 0x02;
		coroutine::start(static_cast<php::callable&>(params[0]));
		return nullptr;
	}
	static php::value run(php::parameters& params) {
		if((status & 0x02) < 0x02) throw php::exception("flame needs at least one coroutine, forget to 'flame\\go()' ?");
		status |= 0x04;
		process_self->run();
		return nullptr;
	}
	static php::value quit(php::parameters& params) {
		if(params.length() > 0) {
			quit_cb.push_back(params[0]);
			return quit_cb.size();
		}else{
			uv_stop(flame::loop);
			return quit_cb.size();
		}
	}
	static php::value output_test(php::parameters& params) {
		for(int i=0;i<1000;++i) {
			std::fprintf(stderr, "==================================================\n");
		}
		return nullptr;
	}
	void init(php::extension_entry& ext) {
		coroutine::prepare();
		// 进程控制
		process::prepare();
		// 基础函数
		ext.add<flame::init>("flame\\init");
		ext.add<flame::go>("flame\\go");
		ext.add<flame::run>("flame\\run");
		ext.add<flame::quit>("flame\\quit");
		ext.add<flame::output_test>("flame\\output_test");
	}

}
