#include <flame/core/string.h>
#include <php/Zend/zend_API.h>

namespace flame::core {

string::~string() {
    zval_ptr_dtor(*this);
}

const char* string::data() const {
    return ZSTR_VAL(zval_get_string(*this));
}

char* string::data() {
    return ZSTR_VAL(zval_get_string(*this));
}

std::size_t string::size() const {
    return ZSTR_LEN(zval_get_string(*this));
}

} // flame::core
