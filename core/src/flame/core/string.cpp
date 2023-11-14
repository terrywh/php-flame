#include "string.h"
#include <php/Zend/zend_API.h>

namespace flame::core {

const char* string::data() const {
    return ZSTR_VAL(z());
}

char* string::data() {
    return ZSTR_VAL(z());
}

std::size_t string::size() const {
    return ZSTR_LEN(z());
}

string::operator zend_string*() const& {
    return zval_get_string(ptr());
}

zend_string* string::z() const& {
    return zval_get_string(ptr());
}

} // flame::core
