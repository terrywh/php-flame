#include "fiber.h"
#include "process_manager.h"

namespace flame {
	process_type_t    process_type;
	process_manager* manager = new process_manager;
	enum parser_status_t {
		PS_TYPE,
		PS_PIPE_BEG,
		PS_PIPE_KEY,
		PS_PIPE,
		PS_TEXT_BEG,
		PS_TEXT_DATA,
		PS_TEXT,
	};
	process_manager::process_manager()
	: worker_i(worker_.end())
	, worker_count(0)
	, exit_(false)
	, pipe_status(PS_TYPE) {

	}
	void process_manager::init(int worker) {
		if(process_type == PROCESS_MASTER) {
			if(worker > 128) {
				worker_count = 128;
			}else if(worker < 0) {
				worker_count = 0;
			}else {
				worker_count = worker;
			}
			// 启动子进程
			for(int i=0;i<worker_count;++i) {
				fork();
			}
		}
	}
	static void proc_kill_timeout(uv_timer_t* tm) {
		delete tm;
		uv_stop(flame::loop);
	}
	static void worker_pipe_cb(uv_signal_t* handle, int sig) {

	}
	static void worker_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		static char buffer[2048];
		buf->base = buffer;
		buf->len  = 2048;
	}
	void process_manager::before_run_loop() {
		if(process_type == PROCESS_MASTER) {
			// 信号监听（主进程退出）
			uv_signal_init(flame::loop, &sa1);
			uv_signal_start(&sa1, master_kill_cb, SIGINT);
			uv_unref(reinterpret_cast<uv_handle_t*>(&sa1));

			uv_signal_init(flame::loop, &sa2);
			uv_signal_start(&sa2, master_kill_cb, SIGTERM);
			uv_unref(reinterpret_cast<uv_handle_t*>(&sa2));
		}else{
			// 通讯管道
			uv_pipe_init(flame::loop, &pipe_, 1);
			uv_pipe_open(&pipe_, 0);
			uv_read_start(reinterpret_cast<uv_stream_t*>(&pipe_), worker_alloc_cb, worker_read_cb);
			// 信号监听（子进程退出）
			uv_signal_init(flame::loop, &sa1);
			uv_signal_init(flame::loop, &sa2);
			uv_signal_start(&sa1, worker_kill_cb, SIGINT);
			uv_signal_start(&sa2, worker_kill_cb, SIGTERM);
			uv_unref(reinterpret_cast<uv_handle_t*>(&sa1));
			uv_unref(reinterpret_cast<uv_handle_t*>(&sa2));
		}
		// 忽略 SIGPIPE 信号
		signal(SIGPIPE, SIG_IGN);
	}
	void process_manager::after_run_loop() {
		signal(SIGPIPE, SIG_DFL);
		while(!exit_cb.empty()) exit_cb.pop();
		// 通知所有子进程退出
		master_kill_cb(nullptr, SIGTERM);
	}
	void process_manager::worker_fork_timeout(uv_timer_t* tm) {
		delete tm;
		if(!manager->exit_) manager->fork();
	}
	void process_manager::worker_exit_cb(uv_process_t* proc, int64_t exit_status, int term_signal) {
		// 若删除的元素恰好是即将使用的分配目标，需要特殊处理
		if(manager->worker_i != manager->worker_.end() && proc == manager->worker_i->second) {
			manager->worker_i = manager->worker_.erase(manager->worker_i);
			if(manager->worker_i == manager->worker_.end()) {
				manager->worker_i = manager->worker_.begin();
			}
		}else{
			manager->worker_.erase(proc->pid);
		}
		delete reinterpret_cast<uv_pipe_t*>(proc->data);
		delete proc;
		if(!manager->exit_ && exit_status != 99) { // 特殊的退出标志用于区别“异常”退出
			uv_timer_t* tm = new uv_timer_t;
			uv_timer_init(flame::loop, tm);
			uv_timer_start(tm, worker_fork_timeout, rand() % 2000 + 3000, 0);
		}/*else if(manager->worker_.empty()) {

		}*/
	}
	static char* of_server(const char* key) {
		php::array  symbol = php::value(&EG(symbol_table));
		php::array& server = symbol["_SERVER"];
		php::string& val   = server[key];
		return val.data();
	}
	void process_manager::fork() {
		uv_process_t* proc = new uv_process_t;
		uv_pipe_t* pipe = new uv_pipe_t;
		char* argv[3];
		uv_process_options_t opts;
		uv_stdio_container_t sios[3];
		size_t cache_size = 128;
		char working_dir[128], executable[128];

		std::memset(&opts, 0, sizeof(uv_process_options_t));
		opts.exit_cb = worker_exit_cb;
		uv_os_setenv("FLAME_CLUSTER_WORKER", "1");
		cache_size = 128;
		uv_cwd(working_dir, &cache_size);
		opts.cwd = working_dir;
		cache_size = 128;
		uv_exepath(executable, &cache_size);
		opts.file = executable;
		opts.args = argv;
		argv[0] = executable;
		argv[1] = of_server("SCRIPT_FILENAME");
		argv[2] = nullptr;
		opts.stdio_count = 3;
		opts.stdio = sios;
		sios[0].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE);
		uv_pipe_init(flame::loop, pipe, 1);
		sios[0].data.stream = reinterpret_cast<uv_stream_t*>(pipe);
		sios[1].flags = UV_INHERIT_FD;
		sios[1].data.fd = 1;
		sios[2].flags = UV_INHERIT_FD;
		sios[2].data.fd = 2;

		int error = uv_spawn(flame::loop, proc, &opts);
		if(error) {
			php::fail("failed to spawn worker process: (%d) %s", error, uv_strerror(error));
			delete proc;
			delete pipe;
			exit(-1);
		}
		proc->data = pipe;
		worker_[proc->pid] = proc;
	}
	void process_manager::worker_send_cb(uv_write_t* req, int status) {
		pipe_send_ctx* ctx = reinterpret_cast<pipe_send_ctx*>(req->data);
		if(ctx->pipe != nullptr) {
			uv_close(reinterpret_cast<uv_handle_t*>(ctx->pipe), nullptr);
		}
		if(ctx->cb != nullptr) {
			ctx->cb(ctx->pipe, ctx->data);
		}
		delete[] reinterpret_cast<char*>(req->data);
		if(status < 0) {
			php::warn("send failed: (%d) %s", status, uv_strerror(status));
			return;
		}
	}
	void process_manager::send_to_worker(const std::string& key, uv_pipe_t* handle, pipe_send_cb cb, void* data, std::map<int, uv_process_t*>::iterator which) {
		uv_stream_t*  pipe = reinterpret_cast<uv_stream_t*>(which->second->data);
		pipe_send_ctx* ctx = reinterpret_cast<pipe_send_ctx*>(new char[sizeof(pipe_send_ctx) + key.length() + 4]);
		ctx->pipe      = handle;
		ctx->data      = data;
		ctx->req.data  = ctx;
		ctx->cb        = cb;
		int error;
		if(handle == nullptr) {
			uv_buf_t buf = uv_buf_init(ctx->buf, sprintf(ctx->buf, "t:%.*s\n", key.length(), key.c_str()));
			error = uv_write(&ctx->req, pipe, &buf, 1, worker_send_cb);
		}else{
			uv_buf_t buf = uv_buf_init(ctx->buf, sprintf(ctx->buf, "s:%.*s\n", key.length(), key.c_str()));
			error = uv_write2(&ctx->req, pipe, &buf, 1,
				reinterpret_cast<uv_stream_t*>(handle), worker_send_cb);
		}
		if(error < 0) {
			php::warn("master send failed: (%d) %s", error, uv_strerror(error));
			return;
		}
	}
	void process_manager::send_to_worker(const std::string& key, uv_pipe_t* handle, pipe_send_cb cb, void* data) {
		if(worker_i == worker_.end() || ++worker_i == worker_.end()) {
			worker_i = worker_.begin();
		}
		if(worker_i == worker_.end()) {
			return;
		}
		send_to_worker(key, handle, cb, data, worker_i);
	}
	void process_manager::worker_read_cb(uv_stream_t* stream, ssize_t n, const uv_buf_t* buf) {
		if(n < 0) {
			if(n == UV_EOF || n == UV_ECANCELED) { // 主进程关闭通道
				worker_kill_cb(nullptr, SIGTERM);
			}else{
				php::fail("worker recv failed: (%d) %s", n, uv_strerror(n));
				uv_close(reinterpret_cast<uv_handle_t*>(stream), nullptr);
				exit(-1);
			}
		}else if(n == 0) {
			return;
		}
		ssize_t i=0;
		while(i<n) {
			char c = buf->base[i];
			switch(manager->pipe_status) {
			case PS_TYPE: // 初始状态（类型标识）
				manager->pipe_buffer.reset();
				if(c == 's') {
					manager->pipe_status = PS_PIPE_BEG;
				}else if(c == 't') {
					manager->pipe_status = PS_TEXT_BEG;
				}else{
					php::fail("illegal pipe data");
					uv_stop(flame::loop);
					goto PARSE_BREAK;
				}
				break;
			case PS_PIPE_BEG:
				manager->pipe_status = PS_PIPE_KEY;
				break;
			case PS_PIPE_KEY:
				if(c == '\n') {
					manager->pipe_status = PS_PIPE;
					goto PARSE_SKIP;
				}else{
					*manager->pipe_buffer.put(1) = c;
				}
				break;
			case PS_PIPE:
				manager->stream_handler();
				manager->pipe_status = PS_TYPE;
				break;
			case PS_TEXT_BEG:
				manager->pipe_status = PS_TEXT;
				break;
			case PS_TEXT_DATA:
				if(c == '\n') {
					manager->pipe_status = PS_TEXT;
					goto PARSE_SKIP;
				}else{
					*manager->pipe_buffer.put(1) = c;
				}
				break;
			case PS_TEXT:
				manager->message_handler();
				manager->pipe_status = PS_TYPE;
				break;
			}
PARSE_NEXT:
			++i;
			continue;
PARSE_BREAK:
			break;
PARSE_SKIP:
			continue;
		}
	}
	void process_manager::set_connection_cb(const std::string& key, pipe_connection_cb cb, void* data) {
		pipe_connection_ctx& ctx = pipe_cctx[key];
		ctx.data = data;
		ctx.cb   = cb;
	}
	void process_manager::stream_handler() {
		std::string key(pipe_buffer.data(), pipe_buffer.size());
		auto ictx = pipe_cctx.find(key);
		if(ictx == pipe_cctx.end()) {
			php::warn("failed to callback accept: connection callback not yet set");
			return;
		}
		ictx->second.cb(
			reinterpret_cast<uv_stream_t*>(&pipe_),
			0, ictx->second.data);
	}
	static void shutdown_cb(uv_shutdown_t* req, int status) {
		uv_close(reinterpret_cast<uv_handle_t*>(req->data), nullptr);
		delete req;
	}
	void process_manager::master_kill_cb(uv_signal_t* handle, int sig) {
		if(manager->exit_) return;
		manager->exit_ = true;
		for(auto i=manager->worker_.begin(); i!= manager->worker_.end(); ++i) {
			// manager->send_to_worker("exit", nullptr, nullptr, i);
			uv_shutdown_t* req = new uv_shutdown_t;
			req->data = i->second->data;
			uv_shutdown(req, reinterpret_cast<uv_stream_t*>(i->second->data),
				shutdown_cb);
		}
		while(!manager->exit_cb.empty()) {
			php::callable& cb = manager->exit_cb.top();
			manager->exit_cb.pop();
			fiber::start(cb);
		}
		// 启动 timer 等待 10000ms 后 uv_stop 停止所有异步流程
		uv_timer_t* tm = new uv_timer_t;
		uv_timer_init(flame::loop, tm);
		uv_timer_start(tm, proc_kill_timeout, 11000, 0);
		uv_unref(reinterpret_cast<uv_handle_t*>(tm)); // loop
	}
	void process_manager::worker_kill_cb(uv_signal_t* handle, int sig) {
		if(manager->exit_) return;
		manager->exit_ = true;
		while(!manager->exit_cb.empty()) {
			php::callable& cb = manager->exit_cb.top();
			manager->exit_cb.pop();
			fiber::start(cb);
		}
		// 启动 timer 等待 10000ms 后 uv_stop 停止所有异步流程
		uv_timer_t* tm = new uv_timer_t;
		uv_timer_init(flame::loop, tm);
		uv_timer_start(tm, proc_kill_timeout, 10000, 0);
		uv_unref(reinterpret_cast<uv_handle_t*>(tm)); // loop
	}
	void process_manager::message_handler() {
		// ??? 提供 worker 自主的进程间通讯

		// if(pipe_buffer.size() >= 4 && std::strncmp(pipe_buffer.data(), "exit", 4) == 0) {
		// 	worker_kill_cb(nullptr, SIGTERM);
		// }else{

		//}
	}
	void process_manager::push_exit_cb(const php::callable& cb) {
		exit_cb.push(cb);
	}
}
