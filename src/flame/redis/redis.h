#pragma once
#include "../../vendor.h"
#include <hiredis/hiredis.h>

namespace flame::redis {
    void declare(php::extension_entry &ext);
    php::value connect(php::parameters &params);
} // namespace flame::redis
