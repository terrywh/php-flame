#ifndef FLAME_CORE_CONSTANT_ENTRY_H
#define FLAME_CORE_CONSTANT_ENTRY_H
#include "value.h"
#include <string>

struct _zend_class_entry;

namespace flame::core {

class constant_entry {
    std::string name_;
    value      value_;
public:
    constant_entry(const std::string& name, const value& v)
    : name_(name), value_(v) {}

    void finalize(int module);
    void finalize(struct _zend_class_entry* ce);
};

constant_entry constant(const std::string& name, const value& v);

} // flame::core

#endif // FLAME_CORE_CONSTANT_ENTRY_H