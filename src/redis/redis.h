#pragma once
#include "../vendor.h"

namespace flame::redis
{
    void declare(php::extension_entry &ext);
    php::value connect(php::parameters &params);
} // namespace flame::redis