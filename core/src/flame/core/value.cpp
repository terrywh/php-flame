#include "value.h"
#include "exception.h"
#include <php/Zend/zend_API.h>
#include <php/Zend/zend_exceptions.h>
#include <boost/assert.hpp>
#include <string_view>

namespace flame::core {

bool type::operator==(const type& r) const {
    BOOST_ASSERT(!fake_ && !r.fake_); // 仅真实类型能进行比较
    return code_ == r.code_;
}

const char* type::name() const {
    return zend_get_type_by_const(code_);
}

type type::undefined { };
type type::null      { IS_NULL };
type type::integer   { IS_LONG };
type type::doubles   { IS_DOUBLE };
type type::string    { IS_STRING };
type type::array     { IS_ARRAY };
type type::object    { IS_OBJECT };
type type::reference { IS_REFERENCE };

type type::boolean   { _IS_BOOL, 1 };
type type::number    { _IS_NUMBER, 1 };
type type::callable  { IS_CALLABLE, 1 };
type type::mixed     { IS_MIXED, 1 };

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

flame::core::type value::value::type() const {
    return flame::core::type{ zval_get_type(*this) };
}

value::operator _zval_struct*() const& {
    return const_cast<_zval_struct*>(reinterpret_cast<const _zval_struct*>(&storage_));
}

_zval_struct* value::ptr() const& {
    return const_cast<_zval_struct*>(reinterpret_cast<const _zval_struct*>(&storage_));
}

bool value::is_boolean() const {
    if (std::uint8_t typ = value::type(); typ == IS_TRUE || typ == IS_FALSE)
        return true;
    return false;
}

bool value::is_callable() const {
    return zend_is_callable(ptr(), IS_CALLABLE_SUPPRESS_DEPRECATIONS, nullptr);
}

value::operator bool() const& {
    if (std::uint8_t typ = value::type(); typ != IS_TRUE && typ != IS_FALSE) {
        flame::core::throw_exception( flame::core::type_error(type::boolean, *this) );
    }
    return Z_LVAL_P(static_cast<zval*>(*this));
}

value::operator int() const& {
    if(value::type() != type::integer) {
        flame::core::throw_exception( flame::core::type_error(type::integer, *this) );
    }
    return Z_LVAL_P(static_cast<zval*>(*this));
}

value::operator std::int64_t() const& {
    if(value::type() != type::integer) {
        flame::core::throw_exception( flame::core::type_error(type::integer, *this) );
    }
    return Z_LVAL_P(static_cast<zval*>(*this));
}
value::operator float() const&{
    if(value::type() != type::doubles) {
        flame::core::throw_exception( flame::core::type_error(type::doubles, *this) );
    }
    return Z_DVAL_P(static_cast<zval*>(*this));
}

value::operator double() const&{
    if(value::type() != type::doubles) {
        flame::core::throw_exception( flame::core::type_error(type::doubles, *this) );
    }
    return Z_DVAL_P(static_cast<zval*>(*this));
}

value::operator std::string_view() const&{
    if(value::type() != type::string) {
        flame::core::throw_exception( flame::core::type_error(type::string, *this) );
    }
    zend_string* str = Z_STR_P(ptr());
    return {ZSTR_VAL(str), ZSTR_LEN(str)};
}

value::operator std::string() const& {
    if(value::type() != type::integer) {
        flame::core::throw_exception( flame::core::type_error(type::string, *this) );
    }
    zend_string* str = Z_STR_P(ptr());
    return {ZSTR_VAL(str), ZSTR_LEN(str)};
}

} // flame::core
