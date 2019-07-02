#pragma once
#include "../../vendor.h"

namespace flame::tcp {
    void declare(php::extension_entry &ext);
    php::value connect(php::parameters &params);
} // namespace flame::tcp
