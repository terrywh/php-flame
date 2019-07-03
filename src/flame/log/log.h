#pragma once
#include "../../vendor.h"

namespace flame::log {
    void declare(php::extension_entry &ext);
    php::value connect(php::parameters& params);
} // namespace flame::log
