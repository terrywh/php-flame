#include "process.h"
#include "worker.h"
#include "coroutine.h"

namespace flame {
	std::deque<php::callable> quit_cb;
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
			flame::loop = uv_default_loop();
		}else{
			process_type = PROCESS_WORKER;
			flame::loop = new uv_loop_t;
			uv_loop_init(flame::loop);
		}
		process_self = new process();
		return process_self;
	}
	static void master_exit_cb(uv_signal_t* handle, int signum) {
		process* proc = reinterpret_cast<process*>(handle->data);
		proc->worker_stop();
		uv_stop(flame::loop);
	}
	static void worker_exit_cb(uv_signal_t* handle, int signum) {
		process* proc = reinterpret_cast<process*>(handle->data);
		uv_stop(flame::loop);
	}
	void process::init() {
		if(process_type == PROCESS_MASTER) {
			uv_signal_init(flame::loop, &signal_);
			uv_signal_start_oneshot(&signal_, master_exit_cb, SIGTERM);
			worker_start();
		}else{
			uv_signal_init(flame::loop, &signal_);
			uv_signal_start_oneshot(&signal_, worker_exit_cb, SIGTERM);
		}
		signal_.data = this;
		uv_unref((uv_handle_t*)&signal_);
	}
	void process::run() {
		if(process_type == PROCESS_MASTER) {
			php::callable("cli_set_process_title").invoke(process_name + " (flame-master)");
		}else{
			php::callable("cli_set_process_title").invoke(process_name + " (flame)");
		}
		uv_run(flame::loop, UV_RUN_DEFAULT);
		if(!EG(exception)) {
			while(!quit_cb.empty()) {
				coroutine::start(quit_cb.back());
				quit_cb.pop_back();
			}
			uv_run(flame::loop, UV_RUN_NOWAIT);
		}
		if(process_type == PROCESS_WORKER) {
			delete flame::loop;
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
