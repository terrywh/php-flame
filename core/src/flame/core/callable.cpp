#include "callable.h"
#include "parameter.h"
#include <php/Zend/zend_API.h>

namespace flame::core {

value callable::operator ()() const& {
    zval return_value;
    auto r = call_user_function(nullptr, nullptr, ptr(), &return_value, 0, nullptr);
    if (ZEND_RESULT_CODE::SUCCESS != r) {
        std::abort();
    }
    return value{&return_value};
}

value callable::operator ()(parameter_list& argv) const& {
    zval return_value, parameters [argv.size()];
    for (int i=0;i<argv.size();++i) {
        parameters[i] = *argv[i];
    }
    auto r = call_user_function(nullptr, nullptr, ptr(), &return_value, argv.size(), parameters);
    if (ZEND_RESULT_CODE::SUCCESS != r) {
        std::abort();
    }
    return value{&return_value};
}

value callable::operator ()(std::initializer_list<value> argv) const& {
    parameter_list list { std::move(argv) };
    return operator ()(list);
}

} // flame::core
