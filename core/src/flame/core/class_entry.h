#ifndef FLAME_CORE_CLASS_ENTRY_H
#define FLAME_CORE_CLASS_ENTRY_H
#include <memory>

struct _zend_class_entry;

namespace flame::core {

class class_entry_finalizer;

class class_entry_base {
protected:
    std::string name_;

public:
    class_entry_base(const std::string& name)
    : name_(name) {}
    struct _zend_class_entry* ce;

    void finalize();
    friend class module_entry;
};

template <class T>
class class_entry : public class_entry_base {

public:
    using class_entry_base::class_entry_base;
    
};

} // flame::core

#endif // FLAME_CORE_CLASS_ENTRY_H