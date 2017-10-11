#include "process_master.h"
#include "process_worker.h"
#include "coroutine.h"
#include "pipe_worker.h"

namespace flame {
	void process_master::init() {
		flame::loop = uv_default_loop();
	}
	void process_master::run() {
		php::callable("cli_set_process_title").invoke(process_name + " (flame-master)");
		for(int i=0;i<process_count;++i) {// 创建子进程
			start_worker();
		}
		process::run();
	}
	void process_master::start_worker() {
		auto w = new pipe_worker(this);
		w->start();
		// worker_[proc->pid] = proc;
		workers_.insert(w);
	}
	void process_master::stop_worker(pipe_worker* w) {
		workers_.erase(w);
		delete w;
	}
}
