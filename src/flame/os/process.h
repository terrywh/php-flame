#pragma once

namespace flame {
namespace os {
namespace cluster {
	class messenger;
}
	class process: public php::class_base {
	public:
		process();
		~process();
		php::value __construct(php::parameters& params);
		php::value kill(php::parameters& params);
		// 必须仅在一个“协程”中等待进程，否则可能导致未知问题
		php::value wait(php::parameters& params);
		php::value send(php::parameters& params);
		php::value ondata(php::parameters& params);
		php::value stdout(php::parameters& params);
		php::value stderr(php::parameters& params);
		// property $pid 进程Id
	private:
		// 生命周期需与 PHP 对象不符，故使用动态分配
		uv_process_t*      proc_;
		cluster::messenger* msg_;
		bool               exit_;
		bool             detach_;
		flame::coroutine*    co_;
		
		php::object      stdout_;
		php::object      stderr_;
		static void exit_cb(uv_process_t* proc, int64_t exit_status, int signal);
		static void string_cb(uv_write_t* handle, int status);
		static void socket_cb(uv_write_t* handle, int status);
		void options_flags(uv_process_options_t& options, php::array& flags);
		
		friend php::value exec(php::parameters& params);
	};
}
}
