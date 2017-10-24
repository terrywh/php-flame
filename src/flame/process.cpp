#include "process.h"
#include "worker.h"

namespace flame {
	process_type_t process_type;
	std::string    process_name;
	uint8_t        process_count;
	process*       process_self;
	uv_loop_t*     loop;
	process* process::prepare() {
		// 确认当前是父进程还是子进程
		char* worker = std::getenv("FLAME_CLUSTER_WORKER");
		if(worker == nullptr) {
			process_type = PROCESS_MASTER;
		}else{
			process_type = PROCESS_WORKER;
		}
		process_self = new process();
		return process_self;
	}
	void process::init() {
		if(process_type == PROCESS_MASTER) {
			flame::loop = uv_default_loop();
		}else{
			flame::loop = new uv_loop_t;
			uv_loop_init(flame::loop);
		}
	}
	void process::run() {
		if(process_type == PROCESS_MASTER) {
			php::callable("cli_set_process_title").invoke(process_name + " (flame-master)");
			worker_start();
		}else{
			php::callable("cli_set_process_title").invoke(process_name + " (flame)");
		}

		uv_run(flame::loop, UV_RUN_DEFAULT);

		if(process_type == PROCESS_WORKER) {
			// 标记退出状态用于确认是否自动重启工作进程
			exit(99);
		}
	}
	void process::worker_start() {
		for(int i=0;i<process_count;++i) {// 创建子进程
			worker* w = new worker(this);
			w->start();
			workers_.insert(w);
		}
	}
	void process::worker_stop() {
		for(auto i=workers_.begin();i!=workers_.end();++i) {
			(*i)->kill();
		}
	}
	void process::on_worker_stop(worker* w) {
		workers_.erase(w);
		delete w;
	}
}
