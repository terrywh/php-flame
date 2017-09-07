#pragma once

namespace flame {
namespace os {
	php::value start_process(php::parameters& params);
	class process: public php::class_base {
	public:
		process();
		~process();
		php::value kill(php::parameters& params);
		php::value wait(php::parameters& params);
		// property $pid 进程Id
	private:
		static void on_exit_cb(uv_process_t* proc, int64_t exit_status, int signal);
		uv_process_t  handle_;
		bool          exit_;
		bool          detach_;
		flame::fiber* fiber_;
		friend php::value start_process(php::parameters& params);
	};
}
}
