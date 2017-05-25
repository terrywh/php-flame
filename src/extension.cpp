#include "vendor.h"
#include "core.h"
#include "task_runner.h"
#include "keeper.h"
#include "net/init.h"
#include "http/init.h"
#include "db/init.h"

void extension_init(php::extension_entry& extension) {
	extension.init(EXTENSION_NAME, EXTENSION_VERSION);
	core::init(extension);
	task_runner::init(extension);
	keeper::init(extension);
	net::init(extension);
	http::init(extension);
	db::init(extension);
}
