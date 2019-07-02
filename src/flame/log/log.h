#pragma once
#include "../../vendor.h"

namespace flame::log {
    class logger;
    // 默认日志记录器
    extern logger* logger_;
    void declare(php::extension_entry &ext);
    php::value connect(php::parameters& params);
} // namespace flame::log
