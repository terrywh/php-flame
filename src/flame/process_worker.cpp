#include "coroutine.h"
#include "process_master.h"
#include "process_worker.h"

namespace flame {
	void process_worker::init() {
		uv_loop_init(&loop_);
		flame::loop = &loop_;
		uv_pipe_init(flame::loop, &pipe_, 1);
		pipe_.data = this;
		// 管道不记录在应用活跃范围
		uv_unref(reinterpret_cast<uv_handle_t*>(&pipe_));
	}

	static void on_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		static char buffer[2048];
		buf->base = buffer;
		buf->len  = 2048;
	}

	void process_worker::run() {
		php::callable("cli_set_process_title").invoke(process_name + " (flame-worker)");
		// 与主进程通讯管道
		uv_pipe_open(&pipe_, 0);
		uv_read_start(reinterpret_cast<uv_stream_t*>(&pipe_), on_alloc, on_pipe_read);

		process::run();
		// 标记退出状态用于确认是否自动重启工作进程
		exit(99);
	}

	void process_worker::on_pipe_read(uv_stream_t* handle, ssize_t n, const uv_buf_t* buf) {
		process_worker* self = static_cast<process_worker*>(handle->data);
		if(n < 0) {
			php::fail("ipc pipe failed: (%d) %s", n, uv_strerror(n));
			uv_close(reinterpret_cast<uv_handle_t*>(handle), nullptr);
			exit(99);
		}else if(n == 0) {
			// wait more data
		}else if(self->pipe_parser_.execute(buf->base, n) != n) {
			php::fail("ipc parse failed");
			uv_close(reinterpret_cast<uv_handle_t*>(handle), nullptr);
			exit(99);
		}
	}
}
