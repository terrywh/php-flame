#include "../vendor.h"
#include "init.h"
#include "lmdb.h"

namespace db {
	void init(php::extension_entry& extension) {
		lmdb::init(extension);
	}
}
