#include "deps.h"
#include "flame.h"
#include "coroutine.h"
#include "worker.h"
#include "process.h"
#include "log/log.h"

namespace flame {
	worker::worker(process* m)
	: master_(m) {
		proc_.data = this;
		uv_timer_init(flame::loop, &timer_);
		timer_.data = this;
	}
	void worker::start() {
		char* argv[3];
		uv_process_options_t opts;
		std::memset(&opts, 0, sizeof(uv_process_options_t));
		opts.flags = UV_PROCESS_WINDOWS_HIDE;
		uv_stdio_container_t sios[3];
		size_t cache_size = 256;
		char working_dir[256], executable[256];
		opts.exit_cb = on_worker_exit;
		uv_os_setenv("FLAME_CLUSTER_WORKER", "1");
		cache_size = 256;
		uv_cwd(working_dir, &cache_size);
		opts.cwd = working_dir;
		cache_size = 256;
		uv_exepath(executable, &cache_size);
		opts.file = executable;
		opts.args = argv;
		argv[0] = executable;
		argv[1] = php::array::server()["SCRIPT_FILENAME"].to_string().data();
		argv[2] = nullptr;
		opts.stdio_count = 3;
		opts.stdio = sios;
		sios[0].flags = UV_INHERIT_FD;
		sios[0].data.fd = 0;
		sios[1].flags = UV_INHERIT_FD;
		sios[1].data.fd = 1;
		sios[2].flags = UV_INHERIT_FD;
		sios[2].data.fd = 2;

		int error = uv_spawn(flame::loop, &proc_, &opts);
		if(error) {
			php::fail("failed to spawn worker process: (%d) %s", error, uv_strerror(error));
			abort();
		}
		// 需要监控子进程并等待其退出完毕
		// uv_unref(reinterpret_cast<uv_handle_t*>(&proc_));
	}
	void worker::kill(int sig) {
		uv_process_kill(&proc_, sig);
	}
	void worker::on_worker_exit(uv_process_t* handle, int64_t exit_status, int term_signal) {
		worker* w = static_cast<worker*>(handle->data);
		if(exit_status == 99 || term_signal == SIGINT) {
			w->master_->on_worker_stop(w);
		}else{
			int timeout = 2000 + (std::rand() % 3000);
			log::default_logger->write(fmt::format("(WARN) worker exit prematurely ({0}), restart in {1}", term_signal, timeout/1000));
			uv_timer_start(&w->timer_, on_worker_restart, timeout, 0);
		}
	}
	void worker::on_worker_restart(uv_timer_t* handle) {
		worker* w = static_cast<worker*>(handle->data);
		w->start();
	}
}
