#pragma once
#include "../../vendor.h"

namespace flame::os {
    
    void declare(php::extension_entry &ext);
    php::value interfaces(php::parameters &params);
    php::value spawn(php::parameters &params);
    php::value exec(php::parameters &params);

} // namespace flame::os
