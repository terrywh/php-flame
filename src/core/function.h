#ifndef PHP_FLAME_CORE_FUNCTION_H
#define PHP_FLAME_CORE_FUNCTION_H

#include <phpext.h>

namespace flame { namespace core {

    class function {
    public:
        static void declare(php::module_entry& entry);
    };

}}


#endif // PHP_FLAME_CORE_FUNCTION_H
