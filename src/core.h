#pragma once
#include "vendor.h"

namespace flame {
    void declare(php::extension_entry& ext);
    php::value select(php::parameters &params);
}