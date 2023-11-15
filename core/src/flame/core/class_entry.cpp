#include "class_entry.h"
#include "constant_entry.h"
#include "function_entry.h"
#include "method_entry.h"
#include "property_entry.h"
#include "module_entry.h"
#include <php/Zend/zend_API.h>
#include <map>
#include <cstring>

namespace flame::core {

struct class_entry_store {
    std::vector<zend_function_entry> method;
    zend_object_handlers            handler;
    std::vector<property_entry>    property;
    std::vector<constant_entry>    constant;
};

static std::map<zend_class_entry*, class_entry_base*> class_registry;

zend_object *class_entry_base::create_object(zend_class_entry *ce) {
    if (auto entry = class_registry.find(ce); entry == class_registry.end()) {
        return zend_objects_new(ce);
    } else {
        zend_object* obj = entry->second->desc_->create_object(ce);
        obj->handlers = &entry->second->store_->handler;
        return obj;
    }
}

void class_entry_base::destroy_object(zend_object *obj) {
    if (auto entry = class_registry.find(obj->ce); entry == class_registry.end()) {
        zend_objects_destroy_object(obj);
    } else {
        entry->second->desc_->destroy_object(obj);
    }
}

class_entry_base::class_entry_base(std::unique_ptr<class_entry_desc> traits)
: desc_(std::move(traits))
, store_(std::make_unique<class_entry_store>()) { // class_entry_base 不会被销毁
}    

class_entry_base& class_entry_base::create(std::unique_ptr<class_entry_desc> traits) {
    return *(new class_entry_base(std::move(traits)));
}

class_entry_base& class_entry_base::operator +(function_entry&& entry) {
    auto& fn = store_->method.emplace_back();
    entry.finalize(&fn);
    fn.flags |= ZEND_ACC_STATIC; // 
    return *this;
}

class_entry_base& class_entry_base::operator +(method_entry&& entry) {
    entry.finalize(&store_->method.emplace_back());
    return *this;
}

class_entry_base& class_entry_base::operator +(property_entry&& entry) {
    store_->property.push_back(std::move(entry));
    return *this;
}

class_entry_base& class_entry_base::operator +(constant_entry&& entry) {
    store_->constant.push_back(std::move(entry));
    return *this;
}

void class_entry_base::finalize() {
    zend_class_entry entry, *ce;

    std::memcpy(&store_->handler, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    store_->handler.offset = desc_->offset();
    store_->handler.dtor_obj = destroy_object;
    if (store_->method.empty()) {
        INIT_CLASS_ENTRY_EX(entry, desc_->name().data(), desc_->name().size(), nullptr);
    } else {
        store_->method.push_back({nullptr});
        INIT_CLASS_ENTRY_EX(entry, desc_->name().data(), desc_->name().size(), store_->method.data());
    }
    
    entry.create_object = create_object;
    ce = zend_register_internal_class(&entry);
    desc_->do_register(ce);
    class_registry[ce] = this; // 此类型的描述信息与注册的类之间建立映射关系

    for (auto& property : store_->property) property.finalize(ce);
    store_->property.clear();
    for (auto& constant : store_->constant) constant.finalize(ce);
    store_->property.clear();
}

} // flame::core
