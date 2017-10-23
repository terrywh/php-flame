#include "../coroutine.h"
#include "os.h"
#include "process.h"

namespace flame {
namespace os {
	void init(php::extension_entry& ext) {
		ext.add<spawn>("flame\\os\\spawn");
		php::class_entry<process> class_process("flame\\os\\process");
		class_process.add(php::property_entry("pid", int(0)));
		class_process.add<&process::kill>("kill");
		class_process.add<&process::wait>("wait");
		ext.add(std::move(class_process));
	}
}
}
