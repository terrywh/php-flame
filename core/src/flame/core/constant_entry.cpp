#include "constant_entry.h"
#include "access_entry.h"
#include <php/Zend/zend_API.h>
#include <php/Zend/zend_constants.h>
#include <boost/assert.hpp>

namespace flame::core {

void constant_entry::finalize(int module) {
    BOOST_ASSERT(value_.of_type() != core::value::type::object && value_.of_type() != core::value::type::reference);

    zend_constant c;
    if (value_.of_type() == core::value::type::string) {
        auto sv = static_cast<std::string_view>(value_);
        ZVAL_STR(&c.value, zend_string_init_interned(sv.data(), sv.size(), true));
    } else {
        ZVAL_COPY_VALUE(&c.value, value_.ptr());
        Z_TRY_ADDREF_P(&c.value); // 防止 value 被释放
    }
    ZEND_CONSTANT_SET_FLAGS(&c, CONST_PERSISTENT, module);
    c.name = zend_string_init_interned(name_.data(), name_.size(), true);
    zend_register_constant(&c);
}

void constant_entry::finalize(struct _zend_class_entry* ce) {
    BOOST_ASSERT(value_.of_type() != core::value::type::object && value_.of_type() != core::value::type::reference);

    zend_string* name = zend_string_init_interned(name_.data(), name_.size(), true);
    zval v;
    if (value_.of_type() == core::value::type::string) {
        auto sv = static_cast<std::string_view>(value_);
        ZVAL_NEW_STR(&v, zend_string_init(sv.data(), sv.size(), true));
    } else {
        ZVAL_COPY_VALUE(&v, value_.ptr());
        Z_TRY_ADDREF_P(&v); // 防止 value 被释放
    }
    zend_declare_class_constant_ex(ce, name, &v, core::public_, nullptr);
}

constant_entry constant(const std::string& name, const value& v) {
    return {name, v};
}

} // flame::core
