#ifndef FLAME_CORE_OBJECT_H
#define FLAME_CORE_OBJECT_H
#include "value.h"
#include "string.h"

struct _zend_class_entry;
struct _zend_object;

namespace flame::core {

class parameter_list;
class object: public value {
public:
    using value::value;
    object(struct _zend_class_entry* ce);
    object(const value& v)
    : value(v) {}
    object(value&& v)
    : value(std::move(v)) {}

    operator struct _zend_object*() const&;
    struct _zend_object* z() const&;

    value call(const core::string& name);
    value call(const core::string& name, parameter_list& argv);
    value call(const core::string& name, std::initializer_list<value> argv);

    value get(const core::string& name);
    void  set(const core::string& name, const value& v);
};

} // flame::core

#endif // FLAME_CORE_OBJECT_H
