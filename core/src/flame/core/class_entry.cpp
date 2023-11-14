#include "class_entry.h"
#include "function_entry.h"
#include "method_entry.h"
#include "module_entry.h"
#include <php/Zend/zend_API.h>
#include <map>
#include <cstring>

namespace flame::core {

struct class_entry_store {
    std::vector<zend_function_entry> method;
    zend_object_handlers handler;
};

static std::map<zend_class_entry*, class_entry_base*> class_registry;

zend_object *class_entry_base::create_object(zend_class_entry *ce) {
    if (auto entry = class_registry.find(ce); entry == class_registry.end()) {
        return zend_objects_new(ce);
    } else {
        zend_object* obj = entry->second->traits_->create_object(ce);
        obj->handlers = &entry->second->store_->handler;
        return obj;
    }
}

void class_entry_base::destroy_object(zend_object *obj) {
    if (auto entry = class_registry.find(obj->ce); entry == class_registry.end()) {
        zend_objects_destroy_object(obj);
    } else {
        entry->second->traits_->destroy_object(obj);
    }
}

class_entry_base::class_entry_base(std::unique_ptr<class_entry_desc> traits)
: traits_(std::move(traits))
, store_(std::make_unique<class_entry_store>()) { // class_entry_base 不会被销毁
}    

class_entry_base& class_entry_base::create(std::unique_ptr<class_entry_desc> traits) {
    return *(new class_entry_base(std::move(traits)));
}

class_entry_base& class_entry_base::operator +(function_entry&& method) {
    method.finalize(&store_->method.emplace_back());
    return *this;
}

class_entry_base& class_entry_base::operator +(method_entry&& method) {
    method.finalize(&store_->method.emplace_back());
    return *this;
}

void class_entry_base::operator+(module_entry& m) {
    m.add(this);
}

void class_entry_base::finalize() {
    zend_class_entry entry;

    std::memcpy(&store_->handler, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    store_->handler.offset = traits_->offset();
    store_->handler.dtor_obj = destroy_object;
    if (store_->method.empty()) {
        INIT_CLASS_ENTRY_EX(entry, traits_->name().data(), traits_->name().size(), nullptr);
    } else {
        store_->method.push_back({nullptr});
        INIT_CLASS_ENTRY_EX(entry, traits_->name().data(), traits_->name().size(), store_->method.data());
    }
    
    entry.create_object = create_object;
    ce = zend_register_internal_class(&entry);
    class_registry[ce] = this; // 此类型的描述信息与注册的类之间建立映射关系
}

} // flame::core
