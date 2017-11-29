#include "../coroutine.h"
#include "os.h"
#include "process.h"
#include "cluster/cluster.h"

namespace flame {
namespace os {
	static php::value executable(php::parameters& params) {
		php::string str(256);
		uv_exepath(str.data(), &str.length());
		return std::move(str);
	}
	void init(php::extension_entry& ext) {
		ext.add<executable>("flame\\os\\executable");
		ext.add<spawn>("flame\\os\\spawn");
		php::class_entry<process> class_process("flame\\os\\process");
		class_process.add(php::property_entry("pid", int(0)));
		class_process.add<&process::__construct>("__construct");
		class_process.add<&process::kill>("kill");
		class_process.add<&process::wait>("wait");
		class_process.add<&process::send>("send");
		class_process.add<&process::ondata>("ondata");
		ext.add(std::move(class_process));

		cluster::init(ext);
	}
}
}
