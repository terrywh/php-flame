#include "vendor.h"
#include "net/init.h"

php::value strerror(php::parameters& params) {
	int ec = params[0];
	return php::value(strerror(ec));
}

php::value sleep(php::parameters& params) {
	mill_msleep(mill_now() + (int)params[0]);
	return nullptr;
}

php::value fork(php::parameters& params) {
	int count = params[0];
	for(int i=0;i<count;++i) {
		pid_t pid = mill_mfork();
		if(pid < 0) {
			std::string message("failed to mfork: ");
			message.append(strerror(errno));
			throw php::exception(message);
		}else if(pid == 0) {
			break;
		}
	}
}

mill_coroutine void coroutine_runner(php::value y) {
	y();
}
php::value go(php::parameters& params) {
	mill_go(coroutine_runner(params[0]));
	return nullptr;
}



void init(php::extension_entry& extension) {
	// php::class_entry<application> mill_application("mill\\application", nullptr, ZEND_ACC_FINAL);
	// mill_application.add<&application::__construct>("__construct");
	// mill_application.add<&application::add>("add");
	// mill_application.add<&application::run>("run");
	// mill_application.add(php::constant_entry("concurrency", static_cast<int>(std::thread::hardware_concurrency())));
	// extension.add(std::move(mill_application));
	//
	// php::class_entry<process> mill_process("mill\\process", nullptr, ZEND_ACC_FINAL);
	// mill_process.add<&process::add>("add");
	// extension.add(std::move(mill_process));
	//
	// php::class_entry<server> mill_server("mill\\server", nullptr, ZEND_ACC_EXPLICIT_ABSTRACT_CLASS);
	// extension.add(std::move(mill_server));
	extension.add<sleep>("mill\\sleep");
	extension.add<fork>("mill\\fork");
	extension.add<go>("mill\\go");
}

extern "C" {
	ZEND_DLEXPORT zend_module_entry* get_module() {
		static php::extension_entry extension(EXTENSION_NAME, EXTENSION_VERSION);
		init(extension);
		net::init(extension);
		return extension;
	}
}
