#pragma once
#include "../../vendor.h"

namespace flame::log {
    enum {
        LEVEL_TRACE,
        LEVEL_DEBUG,
        LEVEL_INFO,
        LEVEL_WARNING,
        LEVEL_ERROR,
        LEVEL_FATAL,
    };
    extern int level;
    void declare(php::extension_entry &ext);
} // namespace flame::log
