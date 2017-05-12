#include "../vendor.h"
#include "init.h"
#include "connection.h"

namespace db {
	void init(php::extension_entry& extension) {
		connection::init(extension);
	}
}
