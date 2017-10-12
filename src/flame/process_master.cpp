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
		worker_i = worker_all.begin();
		process::run();
	}
	void process_master::start_worker() {
		auto w = new pipe_worker(this);
		w->start();
		worker_all.insert(w);
	}
	void process_master::stop_worker(pipe_worker* w) {
		worker_all.erase(w);
		delete w;
	}
	void process_master::send_to_worker_one(std::string msg, uv_stream_t* ss) {
		if(worker_all.size() == 0) return;
		if(worker_i == worker_all.end()) worker_i = worker_all.begin();
		(*worker_i)->send(msg, ss);
	}
	void process_master::send_to_worker_all(std::string msg) {
		for(auto i=worker_all.begin(); i!=worker_all.end(); ++i) {
			(*i)->send(msg);
		}
	}
}
