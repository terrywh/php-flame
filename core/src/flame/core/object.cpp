#include "object.h"
#include "string.h"
#include "parameter.h"
#include "callable.h"
#include <php/Zend/zend_API.h>
#include <boost/assert.hpp>

namespace flame::core {

object::object(zend_class_entry* ce) {
    object_init_ex(ptr(), ce);
    BOOST_ASSERT(Z_REFCOUNT_P(ptr()) == 1);
}

value object::call(std::string_view name) {
    string method{name};
    zval return_value;
    call_user_function(CG(function_table), ptr(), method, &return_value, 0, nullptr);
    handle_exception();
    return {&return_value};
}

value object::call(std::string_view name, parameter_list& argv) {
    string method{name};
    zval return_value, parameters [argv.size()];
    for (int i=0;i<argv.size();++i) {
        parameters[i] = *argv[i];
    }
    call_user_function(CG(function_table), ptr(), method, &return_value, argv.size(), parameters);
    handle_exception();
    return {&return_value};
}

value object::call(std::string_view name, std::initializer_list<value> argv) {
    parameter_list list { std::move(argv) };
    return call(name, list);
}

} // flame::core
