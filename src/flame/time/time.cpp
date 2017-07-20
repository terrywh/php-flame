#include "time.h"
#include "sleep.h"

namespace flame {
namespace time {

	void init(php::extension_entry& ext) {
		ext.add<flame::time::sleep>("flame\\time\\sleep");
	}

}
}