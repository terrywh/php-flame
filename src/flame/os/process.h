#pragma once

namespace flame {
namespace os {
	php::value spawn(php::parameters& params);
	class process: public php::class_base {
	public:
		process();
		~process();
		php::value kill(php::parameters& params);
		// 必须仅在一个“协程”中等待进程，否则可能导致未知问题
		php::value wait(php::parameters& params);
		// property $pid 进程Id
	private:

		uv_process_t  proc_;
		bool          exit_;
		bool          detach_;
		flame::coroutine* co_;
		static void exit_cb(uv_process_t* proc, int64_t exit_status, int signal);
		friend php::value spawn(php::parameters& params);
	};
}
}
