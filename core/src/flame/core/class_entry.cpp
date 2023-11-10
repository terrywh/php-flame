#include "class_entry.h"
#include <php/Zend/zend_API.h>

namespace flame::core {
    
void class_entry_base::finalize() {
    zend_class_entry entry;

    // struct _zend_function_entry methods[1] = { _zend_function_entry {nullptr} };
    INIT_CLASS_ENTRY_EX(entry, name_.data(), name_.size(), nullptr);
    ce = zend_register_internal_class(&entry);
}

} // flame::core
