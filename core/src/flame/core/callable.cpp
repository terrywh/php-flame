#include "callable.h"
#include "parameter.h"
#include <php/Zend/zend_API.h>
#include <php/Zend/zend_exceptions.h>
#include <iostream>

namespace flame::core {
// 参考 zend_try_exception_handler 实现
void handle_exception() {
    if (UNEXPECTED(EG(exception))) {
        if (Z_TYPE(EG(user_exception_handler)) != IS_UNDEF) {
            zend_user_exception_handler();
        } else {
            zend_exception_error(EG(exception), E_ERROR);
        }
        std::abort();
    }
}

value callable::operator ()() const& {
    zval return_value;
    call_user_function(CG(function_table), nullptr, ptr(), &return_value, 0, nullptr);
    handle_exception();
    return value{&return_value};
}

value callable::operator ()(parameter_list& argv) const& {
    zval return_value, parameters [argv.size()];
    for (int i=0;i<argv.size();++i) {
        parameters[i] = *argv[i];
    }
    call_user_function(CG(function_table), nullptr, ptr(), &return_value, argv.size(), parameters);
    handle_exception();
    return value{&return_value};
}

value callable::operator ()(std::initializer_list<value> argv) const& {
    parameter_list list { std::move(argv) };
    return operator ()(list);
}

} // flame::core
