#ifndef CORE_LOG_H
#define CORE_LOG_H

#include "../vendor.h"

namespace core { namespace extension {

    class log {
    public:
        static void declare(php::module_entry& entry);
    };

}}

#endif // CORE_LOG_H
