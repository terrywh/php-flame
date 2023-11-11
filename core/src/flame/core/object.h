#ifndef FLAME_CORE_OBJECT_H
#define FLAME_CORE_OBJECT_H
#include "value.h"

struct _zend_class_entry;

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

    value call(std::string_view name);
    value call(std::string_view name, parameter_list& argv);
    value call(std::string_view name, std::initializer_list<value> argv);
};

} // flame::core

#endif // FLAME_CORE_OBJECT_H
