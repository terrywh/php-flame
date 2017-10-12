#include "coroutine.h"
#include "pipe_worker.h"
#include "process_master.h"

namespace flame {
	pipe_worker::pipe_worker(process_master* m)
	: master_(m) {
		proc_.data = this;
		uv_timer_init(flame::loop, &timer_);
		timer_.data = this;
		// 用于标识启动状态
		pipe_.data = nullptr;
	}
	void pipe_worker::start() {
		char* argv[3];
		uv_process_options_t opts;
		std::memset(&opts, 0, sizeof(uv_process_options_t));
		opts.flags = UV_PROCESS_WINDOWS_HIDE;
		uv_stdio_container_t sios[3];
		size_t cache_size = 128;
		char working_dir[128], executable[128];
		opts.exit_cb = on_worker_exit;
		uv_os_setenv("FLAME_CLUSTER_WORKER", "1");
		cache_size = 128;
		uv_cwd(working_dir, &cache_size);
		opts.cwd = working_dir;
		cache_size = 128;
		uv_exepath(executable, &cache_size);
		opts.file = executable;
		opts.args = argv;
		argv[0] = executable;
		argv[1] = php::array::server()["SCRIPT_FILENAME"].to_string().data();
		argv[2] = nullptr;
		opts.stdio_count = 3;
		opts.stdio = sios;
		sios[0].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE);
		uv_pipe_init(flame::loop, &pipe_, 1);
		pipe_.data = this;
		sios[0].data.stream = reinterpret_cast<uv_stream_t*>(&pipe_);
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
	void pipe_worker::on_worker_exit(uv_process_t* handle, int64_t exit_status, int term_signal) {
		pipe_worker* w = static_cast<pipe_worker*>(handle->data);
		uv_close(reinterpret_cast<uv_handle_t*>(&w->pipe_), nullptr);
		if(exit_status == 99) {
			w->master_->stop_worker(w);
		}else{
			php::warn("worker exit prematurely, will be restarted in 3s");
			uv_timer_start(&w->timer_, on_worker_restart, 3000, 0);
		}
	}
	void pipe_worker::on_worker_restart(uv_timer_t* handle) {
		pipe_worker* w = static_cast<pipe_worker*>(handle->data);
		w->start();
	}
	void pipe_worker::on_send(uv_write_t* handle, int status) {
		// send_context_t* ctx = static_cast<send_context_t*>(handle->data);
		free(handle->data);
	}
	void pipe_worker::send(const std::string& msg, uv_stream_t* ss) {
		if(pipe_.data == nullptr) {
			throw php::exception("unable to send message, worker not started");
		}
		send_context_t* ctx = (send_context_t*)malloc(sizeof(send_context_t) + 4 + msg.length());
		ctx->self = this;
		ctx->req.data = ctx;
		*reinterpret_cast<uint16_t*>(ctx->buf)  = msg.length();
		// 消息格式暂定如下：
		// | 2byte | 1byte | 1byte     | size byte |
		// | size  | type  | reserved  | payload   |
		// 类型 type 定义如下：
		// | 0        | 1          |
		// | 普通消息 | 传递套接字 |
		*reinterpret_cast<uint8_t*>(ctx->buf+2) = ss == nullptr ? 0 : 1;
		std::memcpy(ctx->buf + 4, msg.data(), msg.length());

		uv_buf_t buf {.base = ctx->buf, .len = 4 + msg.length()};
		uv_write2(&ctx->req, reinterpret_cast<uv_stream_t*>(&pipe_), &buf, 1, ss, on_send);
	}
}
