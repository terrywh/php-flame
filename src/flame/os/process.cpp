#include "../fiber.h"
#include "process.h"

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
		if(!ios.empty()) return;
		ios.resize(3);
		ios[0].flags = UV_IGNORE;
		ios[1].flags = UV_IGNORE;
		ios[2].flags = UV_IGNORE;
		options.stdio = ios.data();
		options.stdio_count = ios.size();
	}
	static void options_stdio_close(uv_process_options_t& options) {
		for(int i=0; i<options.stdio_count; ++i) {
			if(options.stdio[i].flags & UV_INHERIT_FD) {
				::close(options.stdio[i].data.fd);
			}
		}
	}
	static void options_flags(std::vector<uv_stdio_container_t>& ios,
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
				options_stdio_init(ios, options);
				ios[1].flags = UV_INHERIT_FD;
				int fd = ::open(i->second.to_string().c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
				ios[1].data.fd = fd;
			}else if(std::strncmp(key.c_str(), "stderr", 6) == 0) {
				options_stdio_init(ios, options);
				ios[2].flags = UV_INHERIT_FD;
				ios[2].data.fd = ::open(i->second.to_string().c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
			}
		}
	}
	php::value start_process(php::parameters& params) {
		uv_process_options_t options;
		std::memset(&options, 0, sizeof(uv_process_options_t));
		options.exit_cb = process::on_exit_cb;
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
		if(params.length() > 4 && params[4].is_array()) {
			options_flags(ios, options, params[4]);
		}
		php::object proc = php::object::create<process>();
		process* proc_ = proc.native<process>();
		if(options.flags & UV_PROCESS_DETACHED) {
			proc_->detach_ = true;
		}
		int error = uv_spawn(flame::loop, &proc_->handle_, &options);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		options_stdio_close(options);
		return std::move(proc);
	}
	process::process()
	: exit_(false)
	, detach_(false)
	, fiber_(nullptr) {
		handle_.data = this;
	}
	process::~process() {
		// 还未退出 而且 未分离父子进程 需要结束进程
		if(!exit_ && !detach_) uv_process_kill(&handle_, SIGKILL);
	}
	php::value process::kill(php::parameters& params) {
		int s = SIGTERM;
		if(params.length() > 0) {
			s = params[0];
		}
		uv_process_kill(&handle_, s);
		return nullptr;
	}
	php::value process::wait(php::parameters& params) {
		if(exit_) return nullptr;
		fiber_ = flame::this_fiber()->push();
		return flame::async();
	}
	void process::on_exit_cb(uv_process_t* handle, int64_t exit_status, int signal) {
		process* proc = reinterpret_cast<process*>(handle->data);
		proc->exit_ = true;
		if(proc->fiber_ != nullptr) {
			proc->fiber_->next(nullptr);
			proc->fiber_ = nullptr;
		}
	}
}
}
