#ifndef FLAME_CORE_CLASS_BASE_H
#define FLAME_CORE_CLASS_BASE_H
#include "class_entry_desc.h"
#include "object.h"
#include "string.h"

struct _zend_class_entry;

namespace flame::core {

class class_static_property {
    struct _zend_class_entry* ce_;
public:
    class_static_property(struct _zend_class_entry* ce);
    value get(const string& name) const;
    void  set(const string& name, const value& prop) const;
};

template <class T>
class class_basic {

protected:
    // 返回当前 CPP 实例对应的 PHP 对象
    object self() const {
        struct _zend_object* obj = class_entry_desc_basic<T>::c2z(const_cast<class_basic<T>*>(this));
        return object{obj};
    }

    static value get_static(const string& name) {
        return class_static_property{class_entry_desc_basic<T>::entry}.get(name);
    }

    static void set_static(const string& name, const value& prop) {
        class_static_property{class_entry_desc_basic<T>::entry}.set(name, prop);
    }
};

} // flame::core

#endif // FLAME_CORE_CLASS_BASE_H
