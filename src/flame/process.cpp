#include "deps.h"
#include "flame.h"
#include "coroutine.h"
#include "process.h"
#include "worker.h"

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
	void process::init() {
		if(process_type == PROCESS_MASTER) {
			uv_signal_init(flame::loop, &sigterm_);
			uv_signal_start_oneshot(&sigterm_, master_exit_cb, SIGTERM);
			uv_signal_init(flame::loop, &sigusr2_);
			uv_signal_start(&sigusr2_, master_usr2_cb, SIGUSR2);
			uv_unref((uv_handle_t*)&sigusr2_);
			sigusr2_.data = this;
			
			worker_start();
		}else{
			uv_signal_init(flame::loop, &sigterm_);
			uv_signal_start_oneshot(&sigterm_, worker_exit_cb, SIGINT);
		}
		uv_unref((uv_handle_t*)&sigterm_);
		sigterm_.data = this;
	}
	void process::run() {
		if(process_type == PROCESS_MASTER) {
			php::callable("cli_set_process_title").invoke(process_name + " (fm)");
		}else{
			php::callable("cli_set_process_title").invoke(process_name + " (fw)");
		}
		uv_run(flame::loop, UV_RUN_DEFAULT);
		// 非错误引发的问题（因异常引发时，进程已经提前结束，不会到达这里）
		if(!quit_cb.empty()) {
			do {
				coroutine::start(quit_cb.back());
				quit_cb.pop_back();
			}while(!quit_cb.empty());
			// 最多 10s 时间必须结束
			int count = 10000;
			while(uv_run(flame::loop, UV_RUN_NOWAIT) != 0 || --count <= 0) {
				usleep(1000);
			}
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
	void process::worker_kill(int sig) {
		for(auto i=workers_.begin();i!=workers_.end();++i) {
			(*i)->kill(sig);
		}
	}
	void process::on_worker_stop(worker* w) {
		workers_.erase(w);
		delete w;
	}
}
