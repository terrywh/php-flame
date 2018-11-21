#pragma once
#include "vendor.h"

namespace flame {
    void declare(php::extension_entry& ext);
    php::value init(php::parameters& params);
    php::value go(php::parameters& params);
    php::value run(php::parameters& params);
}