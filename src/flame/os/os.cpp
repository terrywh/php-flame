#include "../fiber.h"
#include "os.h"
#include "process.h"

namespace flame {
namespace os {
	void init(php::extension_entry& ext) {
		ext.add<start_process>("flame\\os\\start_process");
		php::class_entry<process> class_process("flame\\os\\process");
		class_process.add<&process::kill>("kill");
		class_process.add<&process::wait>("wait");
		ext.add(std::move(class_process));
	}
}
}
