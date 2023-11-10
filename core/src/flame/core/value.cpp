#include "value.h"
#include <php/Zend/zend_API.h>
#include <php/Zend/zend_exceptions.h>
#include "exception.h"

#include <string_view>

namespace flame::core {

bool value::type::operator==(const value::type& r) const {
    return (code_ == r.code_) ||
        // BOOLEAN
        ((code_ == IS_TRUE || code_ == IS_FALSE) && r.code_ ==_IS_BOOL) ||
        (code_ == _IS_BOOL && (r.code_ == IS_TRUE || r.code_ == IS_FALSE)) ||
        // NUMBER
        ((code_ == IS_LONG || code_ == IS_DOUBLE) && r.code_ == _IS_NUMBER) ||
        (code_ == _IS_NUMBER && (r.code_ == IS_LONG || r.code_ == IS_DOUBLE));
}

const char* value::type::name() const {
    return zend_get_type_by_const(code_);
}

value::type value::type::undefined { };
value::type value::type::null      { IS_NULL };
value::type value::type::boolean   { _IS_BOOL };
value::type value::type::integer   { IS_LONG };
value::type value::type::doubles   { IS_DOUBLE };
value::type value::type::number    { _IS_NUMBER };
value::type value::type::string    { IS_STRING };
value::type value::type::array     { IS_ARRAY };
value::type value::type::object    { IS_OBJECT };
value::type value::type::reference { IS_REFERENCE };

value::value() {}

value::value(_zval_struct* value, bool ref) {
    if (ref) {
        ZVAL_MAKE_REF(value);
        zend_reference* ref = Z_REF_P(value);
        GC_ADDREF(ref);
        ZVAL_REF(ptr(), ref);
    } else {
        ZVAL_DUP(ptr(), value);
    }
}

value::value(const value& v) {
    zval* r = v;
    ZVAL_DEREF(r);
    ZVAL_COPY(ptr(), r);
}

value::value(value&& v) {
    ZVAL_UNDEF(ptr());
    std::swap(storage_, v.storage_);
}

value::value(std::nullptr_t) {
    ZVAL_NULL(ptr());
}

value::value(bool b) {
    ZVAL_BOOL(ptr(), b);
}

value::value(int i) {
    ZVAL_LONG(ptr(), i);
}

value::value(std::int64_t i) {
    ZVAL_LONG(ptr(), i);
}

value::value(float f) {
    ZVAL_DOUBLE(ptr(), f);
}

value::value(double f) {
    ZVAL_DOUBLE(ptr(), f);
}

value::value(const char* s)
: value(std::string_view(s)) {}

value::value(std::string_view s) {
    ZVAL_NEW_STR(ptr(), zend_string_init(s.data(), s.size(), 0));
}

value::~value() {
    zval_ptr_dtor(*this);
}

value value::to_reference() {
    value r;
    zval* self = *this;
    ZVAL_MAKE_REF(self); // 自身也转换为引用类型
    zend_reference* ref = Z_REF_P(self);
    GC_ADDREF(ref);
    return r;
}

flame::core::value::type value::value::of_type() const {
    return flame::core::value::type{ zval_get_type(*this) };
}

value::operator _zval_struct*() const& {
    return const_cast<_zval_struct*>(reinterpret_cast<const _zval_struct*>(&storage_));
}

_zval_struct* value::ptr() const& {
    return const_cast<_zval_struct*>(reinterpret_cast<const _zval_struct*>(&storage_));
}

value::operator bool() const& {
    if (value::of_type() == value::type::boolean) {
        flame::core::throw_exception( flame::core::type_error(value::type::boolean, *this) );
    }
    return Z_LVAL_P(static_cast<zval*>(*this));
}

value::operator int() const& {
    if(value::of_type() != value::type::integer) {
        flame::core::throw_exception( flame::core::type_error(value::type::integer, *this) );
    }
    return Z_LVAL_P(static_cast<zval*>(*this));
}

value::operator std::int64_t() const& {
    if(value::of_type() != value::type::integer) {
        flame::core::throw_exception( flame::core::type_error(value::type::integer, *this) );
    }
    return Z_LVAL_P(static_cast<zval*>(*this));
}
value::operator float() const&{
    if(value::of_type() != value::type::doubles) {
        flame::core::throw_exception( flame::core::type_error(value::type::doubles, *this) );
    }
    return Z_DVAL_P(static_cast<zval*>(*this));
}

value::operator double() const&{
    if(value::of_type() != value::type::doubles) {
        flame::core::throw_exception( flame::core::type_error(value::type::doubles, *this) );
    }
    return Z_DVAL_P(static_cast<zval*>(*this));
}

value::operator std::string_view() const&{
    if(value::of_type() != value::type::string) {
        flame::core::throw_exception( flame::core::type_error(value::type::string, *this) );
    }
    zend_string* str = Z_STR_P(ptr());
    return {ZSTR_VAL(str), ZSTR_LEN(str)};
}

value::operator std::string() const& {
    if(value::of_type() != value::type::integer) {
        flame::core::throw_exception( flame::core::type_error(value::type::string, *this) );
    }
    zend_string* str = Z_STR_P(ptr());
    return {ZSTR_VAL(str), ZSTR_LEN(str)};
}

} // flame::core
