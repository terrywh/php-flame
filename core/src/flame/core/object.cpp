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

object::object(struct _zend_object* obj) {
    ZVAL_OBJ(ptr(), obj);
    Z_ADDREF_P(ptr());
}

object::operator _zend_object*() const& {
    return Z_OBJ_P(ptr());
}

_zend_object* object::z() const& {
    return Z_OBJ_P(ptr());
}

value object::call(const core::string& name) {
    string method{name};
    zval return_value;
    call_user_function(CG(function_table), ptr(), method, &return_value, 0, nullptr);
    handle_exception();
    return {&return_value};
}

value object::call(const core::string& name, parameter_list& argv) {
    string method{name};
    zval return_value, parameters [argv.size()];
    for (int i=0;i<argv.size();++i) {
        parameters[i] = *argv[i];
    }
    call_user_function(CG(function_table), ptr(), method, &return_value, argv.size(), parameters);
    handle_exception();
    return {&return_value};
}

value object::call(const core::string& name, std::initializer_list<value> argv) {
    parameter_list list { std::move(argv) };
    return call(name, list);
}

value object::get(const core::string& name) {
    zval rv;
    zend_class_entry* scope = EG(fake_scope) ? EG(fake_scope) : zend_get_executed_scope();
    zval* prop = zend_read_property_ex(scope, z(), name, 0, &rv);
    return {prop};
}

void object::set(const core::string& name, const value& v) {
    zend_class_entry* scope = EG(fake_scope) ? EG(fake_scope) : zend_get_executed_scope();
    zend_object *zobj = Z_OBJ_P(ptr());
    zend_update_property_ex(scope, z(), name, v);
}

} // flame::core
