#include "class_basic.h"
#include <php/Zend/zend_API.h>

namespace flame::core {

class_static_property::class_static_property(struct _zend_class_entry* ce)
: ce_(ce) {}

value class_static_property::get(const string& name) const {
    return {zend_read_static_property_ex(ce_, name, 1)};
}

void   class_static_property::set(const string& name, const value& prop) const {
    zend_update_static_property_ex(ce_, name, prop);
}

} // flame::core
