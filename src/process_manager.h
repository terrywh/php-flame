#pragma once

namespace flame {
	class process_manager {
		typedef void (*pipe_connection_cb)(uv_stream_t* server, int error, void* data);
		typedef struct {
			pipe_connection_cb cb;
			void*              data;
		} pipe_connection_ctx;
		typedef void (*pipe_send_cb)(uv_pipe_t* handle, void* data);
		typedef struct {
			uv_pipe_t*   pipe;
			void*        data;
			uv_write_t   req;
			pipe_send_cb cb;
			char         buf[0];
		} pipe_send_ctx;
	private:
		// uv_process_t.data -> uv_pipe_t 父进程对应管道
		std::map<int, uv_process_t*>           worker_;
		std::map<int, uv_process_t*>::iterator worker_i;
		int                                    worker_count;
		std::stack<php::callable>              exit_cb;
		bool                                   exit_;
		uv_pipe_t                              pipe_; // 子进程 worker 对应管道
		php::buffer                            pipe_buffer;
		int                                    pipe_status;
		std::map<std::string, pipe_connection_ctx> pipe_cctx;

		static void worker_exit_cb(uv_process_t* proc, int64_t exit_status, int term_signal);
		static void worker_send_cb(uv_write_t* req, int status);
		static void worker_kill_cb(uv_signal_t* handle, int sig);
		static void worker_fork_timeout(uv_timer_t* tm);
		static void master_kill_cb(uv_signal_t* handle, int sig);
		static void worker_read_cb(uv_stream_t* stream, ssize_t n, const uv_buf_t* buf);

		void message_handler();
		void stream_handler();
		uv_signal_t sa1, sa2, sa3, sa4;
	public:

		process_manager();

		void init(int worker);
		void before_run_loop();
		void after_run_loop();
		bool is_multiprocess() {
			return worker_count > 0;
		}
		void fork();
		void push_exit_cb(const php::callable& cb);


		void send_to_worker(const std::string& key, uv_pipe_t* handle, pipe_send_cb cb, void* data = nullptr);
		void send_to_worker(const std::string& key, uv_pipe_t* handle, pipe_send_cb cb, void* data, std::map<int, uv_process_t*>::iterator which);
		void set_connection_cb(const std::string& key, pipe_connection_cb cb, void* ctx);
	};
	extern process_manager* manager;
}
