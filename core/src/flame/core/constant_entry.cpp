#include "constant_entry.h"
#include "access_entry.h"
#include <php/Zend/zend_API.h>
#include <php/Zend/zend_constants.h>
#include <boost/assert.hpp>

namespace flame::core {

void constant_entry::finalize(int module) {
    // 参考：ZEND_FUNCTION(define) 实现时似乎存在内存问题
    zend_constant c;
    // ZVAL_COPY(&c.value, value_.ptr());
    // Z_TRY_ADDREF_P(&c.value);
    // ZEND_CONSTANT_SET_FLAGS(&c, CONST_CS, PHP_USER_CONSTANT);
    // c.name = zend_string_init(name_.data(), name_.size(), false);
    auto code = static_cast<std::uint8_t>(value_.type());
    switch (code) {
    case IS_NULL: // type::null
        zend_register_null_constant(name_.data(), name_.size(), CONST_PERSISTENT, module);
        break;
    case IS_TRUE:
    case IS_FALSE:
        zend_register_bool_constant(name_.data(), name_.size(), value_, CONST_PERSISTENT, module);
        break;
    case IS_LONG:
        zend_register_long_constant(name_.data(), name_.size(), value_, CONST_PERSISTENT, module);
        break;
    case IS_DOUBLE:
        zend_register_double_constant(name_.data(), name_.size(), value_, CONST_PERSISTENT, module);
        break;
    case IS_STRING: {
        auto sv = static_cast<std::string_view>(value_);
        zend_register_stringl_constant(name_.data(), name_.size(), sv.data(), sv.size(), CONST_PERSISTENT, module);
        break;
    }
    default:
        zend_error(E_CORE_WARNING, "only basic types can be registered as module constant, '%s'(%d) given", zend_get_type_by_const(code), code);
    }
}

void constant_entry::finalize(struct _zend_class_entry* ce) {
    BOOST_ASSERT(value_.type() != core::type::object && value_.type() != core::type::reference);

    zend_string* name = zend_string_init_interned(name_.data(), name_.size(), true);
    zval v;
    ZVAL_COPY(&v, value_.ptr());
    zend_declare_class_constant_ex(ce, name, &v, core::public_, nullptr);
}

constant_entry constant(const std::string& name, const value& v) {
    return {name, v};
}

} // flame::core
