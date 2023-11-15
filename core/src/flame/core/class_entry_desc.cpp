#include "class_entry_desc.h"
#include <php/Zend/zend_API.h>

namespace flame::core {

struct _zend_object* class_entry_desc::create_object(struct _zend_class_entry* ce) const {
    char* at = reinterpret_cast<char*>(ecalloc(1, size_ + zend_object_properties_size(ce)));
    // 参考 zend_objects_new() 在指定位置初始化
    zend_object* obj = reinterpret_cast<zend_object*>(at + size_);
    zend_object_std_init(obj, ce);
    object_properties_init(obj, ce);
    create(at);
    return obj;
}

void class_entry_desc::destroy_object(struct _zend_object* obj) const {
    char* at = reinterpret_cast<char*>(obj) - size_;  // 计算实际 CPP 对象所在位置
    destroy(at);
    zend_objects_destroy_object(obj);
}

} // flame::core

