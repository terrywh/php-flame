#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "log.h"
#include "logger.h"

namespace flame {
namespace log {
	logger* default_logger = nullptr;
	static php::value fail(php::parameters& params) {
		return default_logger->fail(params);
	}
	static php::value warn(php::parameters& params) {
		return default_logger->warn(params);
	}
	static php::value info(php::parameters& params) {		
		return default_logger->info(params);
	}
	static php::value write(php::parameters& params) {
		return default_logger->write(params);
	}
	static php::value set_output(php::parameters& params) {
		return default_logger->set_output(params);
	}

	void init(php::extension_entry& ext) {
		ext.on_module_startup([] (php::extension_entry& ext) -> bool {
			default_logger = new logger();
			return true;
		});
		ext.on_module_shutdown([] (php::extension_entry& ext) -> bool {
			delete default_logger;
			return true;
		});
		// ---------------------------------------------------------------------
		php::class_entry<logger> class_logger("flame\\log\\logger");
		class_logger.add<&logger::set_output>("set_output");
		class_logger.add<&logger::fail>("fail");
		class_logger.add<&logger::fail>("warn");
		class_logger.add<&logger::fail>("info");
		class_logger.add<&logger::write>("write");
		ext.add(std::move(class_logger));
		// ---------------------------------------------------------------------
		ext.add<set_output>("flame\\log\\set_output");
		ext.add<fail>("flame\\log\\fail");
		ext.add<warn>("flame\\log\\warn");
		ext.add<info>("flame\\log\\info");
		ext.add<write>("flame\\log\\write");
	}
}
}
