#include "../coroutine.h"
#include "dns.h"

namespace flame {
namespace dns {
	static php::value query(php::parameters& params) {
		return flame::async();
	}
	void init(php::extension_entry& ext) {
		ext.add<query>("flame\\dns\\query");
	}
}
}
