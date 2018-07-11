#pragma once

namespace flame {
	class coroutine;
namespace os {
	class process: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value kill(php::parameters& params);
		php::value wait(php::parameters& params);
		php::value stdout(php::parameters& params);
		php::value stderr(php::parameters& params);
	private:
		boost::process::child c_;
		std::shared_ptr<coroutine> co_wait;
		std::future<std::string> out_;
		std::future<std::string> err_;
		bool exit_;
		friend class php::value spawn(php::parameters& params);
		friend class php::value exec(php::parameters& params);
	};
}
}
