#pragma once
#include "logger.h"

namespace flame {
namespace log {
 	extern logger* default_logger;
	void init(php::extension_entry& ext);
}
}
