#include "../coroutine.h"
#include "process.h"

namespace flame {
namespace os {
	void process::declare(php::extension_entry& ext) {
		ext
			.constant({"flame\\os\\SIGTERM", SIGTERM})
			.constant({"flame\\os\\SIGKILL", SIGKILL})
			.constant({"flame\\os\\SIGINT", SIGINT})
			.constant({"flame\\os\\SIGUSR1", SIGUSR1})
			.constant({"flame\\os\\SIGUSR2", SIGUSR2});

		php::class_entry<process> class_process("flame\\os\\process");
		class_process
			.property({"pid", 0})
			.method<&process::kill>("kill", {
				{"signal", php::TYPE::INTEGER, false, true}
			})
			.method<&process::wait>("wait")
			.method<&process::stdout>("stdout")
			.method<&process::stderr>("stderr");
		ext.add(std::move(class_process));
	}
	php::value process::kill(php::parameters& params) {
		if(params.size() > 0) {
			::kill(get("pid"), params[0].to_integer());
		}else{
			::kill(get("pid"), SIGTERM);
		}
		return nullptr;
	}
	php::value process::wait(php::parameters& params) {
		if(exit_) {
			return nullptr;
		}else{
			co_wait = coroutine::current;
			return coroutine::async();
		}
	}
	php::value process::stdout(php::parameters& params) {
		if(!exit_) {
			throw php::exception(zend_ce_exception, "process hadn't exit (maybe you should wait on it?)");
		}
		return out_.get();
	}
	php::value process::stderr(php::parameters& params) {
		if(!exit_) {
			throw php::exception(zend_ce_exception, "process hadn't exit (maybe you should wait on it?)");
		}
		return out_.get();
	}
}
}
