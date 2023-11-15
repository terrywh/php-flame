#ifndef FLAME_CORE_PROPERTY_ENTRY_H
#define FLAME_CORE_PROPERTY_ENTRY_H

#include "access_entry.h"
#include "value.h"

struct _zend_class_entry;

namespace flame::core {

class property_entry {
    std::string   name_;
    value        value_;
    access_entry   acc_;
    
public:
    property_entry(const std::string& name, const value& def);
    property_entry& operator %(access_entry::modifier mod);
    void finalize(struct _zend_class_entry* ce);
};

property_entry property(const std::string& name, const value& v);

} // flame::core

#endif // FLAME_CORE_PROPERTY_ENTRY_H
