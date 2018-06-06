#include "deps.h"
#include "flame.h"
#include "coroutine.h"
#include "process.h"
#include "worker.h"

namespace flame {
	process_type_t process_type;
	std::string    process_name;
	uint8_t        process_count;
	process*       process_self;
	uv_loop_t*     loop;
	void process::prepare() {
		// 确认当前是父进程还是子进程
		char* worker = std::getenv("FLAME_CLUSTER_WORKER");
		flame::loop = uv_default_loop();
		if(worker == nullptr) {
			process_type = PROCESS_MASTER;
		}else{
			uv_loop_fork(flame::loop);
			process_type = PROCESS_WORKER;
		}
	}
	static void master_exit_cb(uv_signal_t* handle, int signum) {
		process* self = reinterpret_cast<process*>(handle->data);
		self->worker_kill(SIGINT); // SIGINT 按“正常退出”处理
		uv_stop(flame::loop);
	}
	static void worker_exit_cb(uv_signal_t* handle, int signum) {
		process* self = reinterpret_cast<process*>(handle->data);
		uv_stop(flame::loop);
	}
	static void master_usr2_cb(uv_signal_t* handle, int signum) {
		process* proc = reinterpret_cast<process*>(handle->data);
		proc->worker_kill(SIGUSR2);
	}
	process::process() {
		if(process_type == PROCESS_MASTER) {
			worker_start();
			uv_signal_init(flame::loop, &sigterm_);
			uv_signal_start_oneshot(&sigterm_, master_exit_cb, SIGTERM);
			uv_signal_init(flame::loop, &sigusr2_);
			uv_signal_start(&sigusr2_, master_usr2_cb, SIGUSR2);
			uv_unref((uv_handle_t*)&sigusr2_);
			sigusr2_.data = this;
		}else{
			uv_signal_init(flame::loop, &sigterm_);
			uv_signal_start_oneshot(&sigterm_, worker_exit_cb, SIGINT);
		}
		uv_unref((uv_handle_t*)&sigterm_);
		sigterm_.data = this;
	}
	void process::run() {
		if(process_type == PROCESS_MASTER) {
			php::callable("cli_set_process_title").invoke({process_name + " (flame master)"});
		}else{
			php::callable("cli_set_process_title").invoke({process_name + " (flame worker)"});
		}
		uv_run(flame::loop, UV_RUN_DEFAULT);
		// 非错误引发的问题（因异常引发时，进程已经提前结束，不会到达这里）
		usleep(10000);
		if(process_type == PROCESS_WORKER) {
			exit(99); // 标记正常退出: 无需重启
		}
	}
	void process::worker_start() {
		for(int i=0;i<process_count;++i) {// 创建子进程
			worker* w = new worker(this, i+1);
			w->start();
		}
	}
	void process::worker_kill(int sig) {
		for(auto i=workers_.begin();i!=workers_.end();++i) {
			(*i)->kill(sig);
		}
	}
	void process::on_worker_start(worker* w) {
		workers_.insert(w);
	}
	void process::on_worker_stop(worker* w) {
		workers_.erase(w);
		delete w;
	}
}
