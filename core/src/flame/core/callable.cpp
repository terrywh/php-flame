#include "callable.h"
#include "parameter.h"
#include "class_entry_desc.h"
#include "closure.h"
#include <php/Zend/zend_API.h>
#include <php/Zend/zend_exceptions.h>


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

template <class S>
static void create_closure(zval* v, rome::delegate<S>&& fn) {
    object_init_ex(v, class_entry_desc_basic<closure>::ce);
    closure* cpp = class_entry_desc_basic<closure>::z2c(Z_OBJ_P(v));
    cpp->set_delegate(std::move(fn));
}

callable::callable(rome::delegate<void ()>&& fn) {
    create_closure<void ()>(ptr(), std::move(fn));
}

callable::callable(rome::delegate<value ()>&& fn) {
    create_closure<value ()>(ptr(), std::move(fn));
}

callable::callable(rome::delegate<void (parameter_list&)>&& fn) {
    create_closure<void (parameter_list&)>(ptr(), std::move(fn));
}

callable::callable(rome::delegate<value (parameter_list&)>&& fn) {
    create_closure<value (parameter_list&)>(ptr(), std::move(fn));
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
