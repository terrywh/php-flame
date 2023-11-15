#include "constant_entry.h"
#include "access_entry.h"
#include <php/Zend/zend_API.h>
#include <php/Zend/zend_constants.h>
#include <boost/assert.hpp>

namespace flame::core {

void constant_entry::finalize(int module) {
    
    BOOST_ASSERT(value_.of_type() != core::value::type::object && value_.of_type() != core::value::type::reference);
    Z_TRY_ADDREF_P(value_.ptr());

    zend_constant c;
    c.name = zend_string_init_interned(name_.data(), name_.size(), true);
    ZVAL_COPY_VALUE(&c.value, value_.ptr());
    ZEND_CONSTANT_SET_FLAGS(&c, CONST_PERSISTENT, module);
    zend_register_constant(&c);
}

void constant_entry::finalize(struct _zend_class_entry* ce) {
    zend_string* name = zend_string_init_interned(name_.data(), name_.size(), true);
    Z_TRY_ADDREF_P(value_.ptr());
    zval v;
    ZVAL_COPY_VALUE(&v, value_.ptr());
    zend_declare_class_constant_ex(ce, name, &v, core::public_, nullptr);
}

constant_entry constant(const std::string& name, const value& v) {
    return {name, v};
}

} // flame::core
