#pragma once
#include "../vendor.h"

namespace flame::tcp
{
    void declare(php::extension_entry &ext);
    php::value connect(php::parameters &params);
    std::pair<std::string, std::string> addr2pair(const std::string& addr);
} // namespace flame::tcp