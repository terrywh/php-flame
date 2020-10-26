#ifndef PHP_FLAME_CORE_FUNCTION_INIT_OPTIONS_H
#define PHP_FLAME_CORE_FUNCTION_INIT_OPTIONS_H

#include <phpext.h>

namespace flame { namespace core {
    class function_init_options {
    public:
        function_init_options(php::parameters& params);
    };
}}

#endif // PHP_FLAME_CORE_FUNCTION_INIT_OPTIONS_H
