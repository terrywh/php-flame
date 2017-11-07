#include "../coroutine.h"
#include "process.h"
#include "../net/unix_socket.h"
#include "../net/udp_socket.h"
#include "../net/tcp_socket.h"

namespace flame {
namespace os {
	static void options_argv(std::vector<char*>& args, uv_process_options_t& options, php::array& argv) {
		for(auto i=argv.begin();i!=argv.end();++i) {
			args.push_back( i->second.to_string().data() );
		}
		args.push_back(nullptr);
		options.args = args.data();
	}
	static void options_envs(std::vector<char*>& envs, uv_process_options_t& options, php::array& env) {
		static char envb[4096], *envp = envb;
		for(auto i=env.begin();i!=env.end();++i) {
			envs.push_back(envp);
			envp += sprintf(envp, "%s=%s\0",
				i->first.to_string().c_str(),
				i->second.to_string().c_str());
		}
		envs.push_back(nullptr);
		options.env = envs.data();
	}
	static void options_stdio_init(std::vector<uv_stdio_container_t>& ios, uv_process_options_t& options) {
		ios.resize(3);
		ios[0].flags = UV_INHERIT_FD;
		ios[0].data.fd = 0;
		ios[1].flags = UV_INHERIT_FD;
		ios[1].data.fd = 1;
		ios[2].flags = UV_INHERIT_FD;
		ios[2].data.fd = 2;
		options.stdio = ios.data();
		options.stdio_count = ios.size();
	}
	static void options_stdio_close(uv_process_options_t& options) {
		for(int i=0; i<options.stdio_count; ++i) {
			if((options.stdio[i].flags & UV_INHERIT_FD) && options.stdio[i].data.fd > 2) {
				::close(options.stdio[i].data.fd);
			}
		}
	}
	void process::options_flags(std::vector<uv_stdio_container_t>& ios,
		uv_process_options_t& options, php::array& flags) {

		for(auto i=flags.begin();i!=flags.end();++i) {
			php::string& key = i->first.to_string();
			if(std::strncmp(key.c_str(), "uid", 3) == 0) {
				options.flags |= UV_PROCESS_SETUID;
				options.uid = i->second.to_long();
			}else if(std::strncmp(key.c_str(), "gid", 3) == 0) {
				options.flags |= UV_PROCESS_SETGID;
				options.gid = i->second.to_long();
			}else if(std::strncmp(key.c_str(), "detach", 6) == 0) {
				options.flags |= UV_PROCESS_DETACHED;
			}else if(std::strncmp(key.c_str(), "stdout", 6) == 0) {
				ios[1].flags = UV_INHERIT_FD;
				int fd = ::open(i->second.to_string().c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
				ios[1].data.fd = fd;
			}else if(std::strncmp(key.c_str(), "stderr", 6) == 0) {
				ios[2].flags = UV_INHERIT_FD;
				ios[2].data.fd = ::open(i->second.to_string().c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
			}else if(std::strncmp(key.c_str(), "ipc", 3) == 0) {
				ios[0].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE);
				uv_pipe_init(flame::loop, &pipe_, 1);
				pipe_.data = this;
				ios[0].data.stream = reinterpret_cast<uv_stream_t*>(&pipe_);
			}
		}
		if((options.flags & UV_PROCESS_DETACHED) > 0 && !ios.empty()) {
			throw php::exception("cannot redirect stdout/stderr or create pipe when process detached is enabled");
		}
	}
	php::value spawn(php::parameters& params) {
		php::object obj = php::object::create<process>();
		process*    cpp = obj.native<process>();
		cpp->__construct(params);
		return std::move(obj);
	}
	process::process()
	: exit_(false)
	, detach_(false)
	, co_(nullptr) {
		proc_.data = this;
		// 标识管道是否建立
		pipe_.data = nullptr;
	}
	process::~process() {
		// 还未退出 而且 未分离父子进程 需要结束进程
		if(!exit_ && !detach_) uv_process_kill(&proc_, SIGKILL);
	}
	php::value process::__construct(php::parameters& params) {
		uv_process_options_t options;
		std::memset(&options, 0, sizeof(uv_process_options_t));
		options.exit_cb = process::exit_cb;
		// 1. file
		php::string& file = params[0];
		options.file = file.data();
		// 2. args
		std::vector<char*> args;
		args.push_back(file.data());
		if(params.length() > 1 && params[1].is_array()) {
			options_argv(args, options, params[1]);
		}
		options.args = args.data();
		// 3. env
		std::vector<char*> envs;
		if(params.length() > 2 && params[2].is_array()) {
			options_envs(envs, options, params[2]);
		}
		// 4. cwd
		if(params.length() > 3 && params[3].is_string()) {
			php::string& cwd = params[3];
			options.cwd = cwd.c_str();
		}
		// 5. options
		std::vector<uv_stdio_container_t> ios;
		options_stdio_init(ios, options);
		if(params.length() > 4 && params[4].is_array()) {
			options_flags(ios, options, params[4]);
		}
		if(options.flags & UV_PROCESS_DETACHED) {
			detach_ = true;
		}
		int error = uv_spawn(flame::loop, &proc_, &options);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		prop("pid") = proc_.pid;
		options_stdio_close(options);
		return nullptr;
	}
	php::value process::kill(php::parameters& params) {
		int s = SIGTERM;
		if(params.length() > 0) {
			s = params[0];
		}
		uv_process_kill(&proc_, s);
		return nullptr;
	}
	php::value process::wait(php::parameters& params) {
		if(exit_) return nullptr;
		co_ = coroutine::current;
		return flame::async();
	}
	void process::exit_cb(uv_process_t* handle, int64_t exit_status, int signal) {
		process* proc = reinterpret_cast<process*>(handle->data);
		proc->exit_ = true;
		if(proc->co_ != nullptr) {
			proc->co_->next();
			proc->co_ = nullptr;
		}
	}

	typedef struct send_request_t {
		int         head;
		php::string data;
		php::value  ref;
		php::object sock;
		uv_write_t  req;
	} send_request_t;

	void process::message_cb(uv_write_t* handle, int status) {
		send_request_t* ctx = static_cast<send_request_t*>(handle->data);
		delete ctx;
	}
	php::value process::send_message(php::parameters& params) {
		// 分离的进程 或 未建立 ipc 通道无法进行消息传递
		if(detach_ || pipe_.data == nullptr) return nullptr;
		// 考虑使用简单的协议封装传输，目前需要标识出携带 HANDLE 传递的情况
		send_request_t* ctx = new send_request_t {
			.head = (int)params[0].length(),
			.data = params[0],
			.ref  = this,
		};
		ctx->ref = this; // 当前对象引用，防止丢失
		ctx->req.data = ctx;

		uv_buf_t data[] = {
			{.base = (char*)&ctx->head, .len = 4},
			{.base = ctx->data.data(),  .len = ctx->data.length()}
		};
		uv_write(&ctx->req, (uv_stream_t*)&pipe_, data, 2, message_cb);
		return flame::async();
	}

	void process::socket_cb(uv_write_t* handle, int status) {
		send_request_t* ctx = static_cast<send_request_t*>(handle->data);
		ctx->sock.call("close"); // 当前进程中的连接需要关闭
		delete ctx;
	}

	php::value process::send_socket(php::parameters& params) {
		// 理论上仅允许传递 unix_socket
		php::object& obj = params[1];
		uv_stream_t* sock;
		if(obj.is_instance_of<net::unix_socket>()) {
			sock = (uv_stream_t*)&obj.native<net::unix_socket>()->impl->stream;
		}else if(obj.is_instance_of<net::udp_socket>()) {
			sock = (uv_stream_t*)&obj.native<net::udp_socket>()->stream;
		}else if(obj.is_instance_of<net::tcp_socket>()) {
			sock = (uv_stream_t*)&obj.native<net::tcp_socket>()->impl->stream;
		}else{
			throw php::exception("only socket object can be sent");
		}
		send_request_t* ctx = new send_request_t {
			.head = 0x01000000,
			.data = nullptr,
			.ref  = this,
			.sock = obj,
		};
		ctx->ref = this;
		ctx->req.data = ctx;
		uv_buf_t data[] = {
			{.base = (char*)&ctx->head, .len = 4},
		};
		uv_write2(&ctx->req, (uv_stream_t*)&pipe_, data, 2, sock, socket_cb);
		return flame::async();
	}

}
}
