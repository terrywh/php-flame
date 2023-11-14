#ifndef FLAME_CORE_CLASS_ENTRY_H
#define FLAME_CORE_CLASS_ENTRY_H
#include "class_entry_desc.h"
#include <memory>

struct _zend_object;
struct _zend_class_entry;

namespace flame::core {

class module_entry;
class function_entry;
class method_entry;
struct class_entry_store;

class class_entry_base {
    std::unique_ptr<class_entry_desc> traits_;
    std::shared_ptr<class_entry_store> store_;
    
    class_entry_base(std::unique_ptr<class_entry_desc> traits);
    static struct _zend_object *create_object(struct _zend_class_entry *ce);
    static void destroy_object(struct _zend_object *obj);
public:
    static class_entry_base& create(std::unique_ptr<class_entry_desc> traits);
    struct _zend_class_entry* ce;
    
    class_entry_base& operator +(function_entry&& method);
    class_entry_base& operator +(method_entry&& method);

    void operator +(module_entry& module);

    void finalize();
};

template <class T>
class_entry_base& class_entry(const std::string& name) {
    return class_entry_base::create(std::make_unique<class_entry_desc_basic<T>>(name));
}

} // flame::core

#endif // FLAME_CORE_CLASS_ENTRY_H