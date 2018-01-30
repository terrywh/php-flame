#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "../net/unix_socket.h"
#include "process.h"
#include "cluster/cluster.h"
#include "cluster/messenger.h"

namespace flame {
namespace os {
	static void options_argv(std::vector<char*>& args, uv_process_options_t& options, php::array& argv) {
		for(auto i=argv.begin();i!=argv.end();++i) {
			args.push_back( i->second.to_string().data() );
		}
		args.push_back(nullptr);
		options.args = args.data();
	}
	static void options_stdio_close(uv_process_options_t& options) {
		for(int i=0; i<options.stdio_count; ++i) {
			if((options.stdio[i].flags & UV_INHERIT_FD) && options.stdio[i].data.fd > 2) {
				::close(options.stdio[i].data.fd);
			}
		}
	}
	void process::options_flags(uv_process_options_t& options, php::array& flags) {
		for(auto i=flags.begin();i!=flags.end();++i) {
			php::string& key = i->first.to_string();
			if(std::strncmp(key.c_str(), "cwd", 3) == 0) {
				// 工作目录
				php::string str = i->second.to_string();
				options.cwd = str.c_str();
			}else if(std::strncmp(key.c_str(), "uid", 3) == 0) {
				// 工作用户
				options.flags |= UV_PROCESS_SETUID;
				options.uid = i->second.to_long();
			}else if(std::strncmp(key.c_str(), "gid", 3) == 0) {
				// 工作组
				options.flags |= UV_PROCESS_SETGID;
				options.gid = i->second.to_long();
			}else if(std::strncmp(key.c_str(), "detach", 6) == 0) {
				// 分离父子进程
				options.flags |= UV_PROCESS_DETACHED;
				detach_ = true;
			}else if(std::strncmp(key.c_str(), "stdout", 6) == 0) {
				options.flags &= ~UV_PROCESS_DETACHED;
				detach_ = false;
				// 标准输出重定向
				php::string str = i->second.to_string();
				if(std::strncmp(str.c_str(), "pipe", 4) == 0) {
					stdout_ = php::object::create<net::unix_socket>();
					net::unix_socket* cpp = stdout_.native<net::unix_socket>();
					cpp->flags = net::unix_socket::CAN_READ;
					options.stdio[1].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE);
					options.stdio[1].data.stream = reinterpret_cast<uv_stream_t*>(cpp->sck);
				}else{
					options.stdio[1].flags = UV_INHERIT_FD;
					options.stdio[1].data.fd = ::open(str.to_string().c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);;
				}
			}else if(std::strncmp(key.c_str(), "stderr", 6) == 0) {
				options.flags &= ~UV_PROCESS_DETACHED;
				detach_ = false;
				// 错误输出重定向
				php::string str = i->second.to_string();
				if(std::strncmp(str.c_str(), "pipe", 4) == 0) {
					stderr_ = php::object::create<net::unix_socket>();
					net::unix_socket* cpp = stdout_.native<net::unix_socket>();
					cpp->flags = net::unix_socket::CAN_READ;
					options.stdio[2].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE);
					options.stdio[2].data.stream = reinterpret_cast<uv_stream_t*>(cpp->sck);
				}else{
					options.stdio[2].flags = UV_INHERIT_FD;
					options.stdio[2].data.fd = ::open(i->second.to_string().c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
				}
			}else if(std::strncmp(key.c_str(), "ipc", 3) == 0) {
				options.flags &= ~UV_PROCESS_DETACHED;
				detach_ = false;
				// 在标准输入上面建立 IPC 通讯管道（内置协议支持，实现 数据和套接字 传输）
				options.stdio[0].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE);
				msg_ = new cluster::messenger(-1); // 创建管道
				options.stdio[0].data.stream = reinterpret_cast<uv_stream_t*>(&msg_->pipe_);
			}
		}
	}
	process::process()
	: msg_(nullptr)
	, exit_(false)
	, detach_(false)
	, co_(nullptr) {
		proc_ = (uv_process_t*)malloc(sizeof(uv_process_t));
		proc_->data = this;
	}
	process::~process() {
		// 还未退出 而且 未分离父子进程 需要结束进程
		if(!exit_ && !detach_) uv_process_kill(proc_, SIGKILL);
		uv_close((uv_handle_t*)proc_, flame::free_handle_cb);
		if(msg_ != nullptr) {
			msg_->close(); // 自主释放
		}
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
			php::array& argv = params[1];
			for(auto i=argv.begin();i!=argv.end();++i) {
				args.push_back( i->second.to_string().data() );
			}
		}
		args.push_back(nullptr);
		options.args = args.data();
		// 默认共享标准IO
		uv_stdio_container_t ios[3];
		ios[0].flags = UV_INHERIT_FD;
		ios[0].data.fd = 0;
		ios[1].flags = UV_INHERIT_FD;
		ios[1].data.fd = 1;
		ios[2].flags = UV_INHERIT_FD;
		ios[2].data.fd = 2;
		options.stdio = ios;
		options.stdio_count = 3;
		// 环境变量容器
		static char envb[4096],
			*envp = envb;
		std::vector<char*> envs;
		// 3. options
		if(params.length() > 2 && params[2].is_array()) {
			php::array& flags = params[2];
			php::array env = flags.at("env", 3);
			
			if(env.is_array()) {
				for(auto i=env.begin();i!=env.end();++i) {
					envs.push_back(envp);
					envp += sprintf(envp, "%s=%s\0",
						i->first.to_string().c_str(),
						i->second.to_string().c_str());
				}
				envs.push_back(nullptr);
				options.env = envs.data();
			}
			options_flags(options, flags);
		}
		int error = uv_spawn(flame::loop, proc_, &options);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		prop("pid") = proc_->pid;
		options_stdio_close(options);
		if(msg_) msg_->start();
		return nullptr;
	}
	php::value process::kill(php::parameters& params) {
		int s = SIGTERM;
		if(params.length() > 0) {
			s = params[0];
		}
		uv_process_kill(proc_, s);
		exit_ = true;
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
	php::value process::send(php::parameters& params) {
		// 分离的进程 或 未建立 ipc 通道无法进行消息传递
		if(detach_ || msg_ == nullptr) return nullptr;
		msg_->send(params);
		return flame::async();
	}
	php::value process::ondata(php::parameters& params) {
		if(detach_ || msg_ == nullptr) return nullptr;
		msg_->cb_string = params[0];
		if(msg_->cb_string.is_callable()) {
			msg_->cb_type |= 0x01;
		}else{
			throw php::exception("callale is required as string handler");
		}
		return nullptr;
	}
	php::value process::stdout(php::parameters& params) {
		if(detach_ || stdout_.is_empty()) throw php::exception("stdout pipe does not exist");
		return stdout_;
	}
	php::value process::stderr(php::parameters& params) {
		if(detach_ || stdout_.is_empty()) throw php::exception("stderr pipe does not exist");
		return stdout_;
	}
}
}
