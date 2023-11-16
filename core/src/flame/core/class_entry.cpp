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
    std::vector<zend_function_entry>  method;
    zend_object_handlers             handler;
    std::vector<property_entry>     property;
    std::vector<constant_entry>     constant;
    std::unique_ptr<class_entry_provider> extend;
    std::vector<std::unique_ptr<class_entry_provider>> implement;
};

static std::map<zend_class_entry*, class_entry_base*> class_registry;

zend_object *class_entry_base::create_object(zend_class_entry *ce) {
    // 这里直接使用了全局的 MAP 来映射对应的类型实例，原实现通过按类形实现的对应静态类型（不能隐藏 Zend 依赖）
    // 分析 PHP-CPP 的实现时通过在 doc_comment 中保存了上述实例指针，可能效率稍高但有些不雅观
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

void class_entry_base::extends(std::unique_ptr<class_entry_provider> entry) {
    store_->extend = std::move(entry);
}

void class_entry_base::implements(std::unique_ptr<class_entry_provider> entry) {
    store_->implement.push_back(std::move(entry));
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

class_entry_base& class_entry_base::extends(const std::string& name) {
    extends(std::make_unique<class_entry_provider_php>(name));
    return *this;
}

class_entry_base& class_entry_base::implements(const std::string& name) {
    implements(std::make_unique<class_entry_provider_php>(name));
    return *this;
}

class_entry_base& class_entry_base::operator %(access_entry::modifier m) {
    acc_ % m;
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
    if (!store_->extend) {
        ce = zend_register_internal_class(&entry);
    } else if (zend_class_entry* pe = store_->extend->provide(); pe) {
        ce = zend_register_internal_class_ex(&entry, pe);
    } else {
        zend_error(E_CORE_WARNING, "class '%s' failed to extends parent '%s': class not found",
            desc_->name().data(), store_->extend->name().data());
        ce = zend_register_internal_class(&entry);
    }
    desc_->do_register(ce, this);
    class_registry[ce] = this; // 此类型的描述信息与注册的类之间建立映射关系

    ce->ce_flags |= acc_.finalize();

    for (auto& interface : store_->implement) {
        if (zend_class_entry* ie = interface->provide(); ie) zend_class_implements(ce, 1, ie);
        else zend_error(E_CORE_WARNING, "class '%s' failed to implement interface '%s': interface not found",
            desc_->name().data(), interface->name().data());
    }

    for (auto& property : store_->property) property.finalize(ce);
    store_->property.clear();
    for (auto& constant : store_->constant) constant.finalize(ce);
    store_->property.clear();
}

struct _zend_class_entry* class_entry_provider_php::provide() const {
    zend_string* n = zend_string_init(name_.data(), name_.size(), false);
    zend_class_entry* ce = zend_lookup_class(n);
    zend_string_release(n);
    return ce;
}

} // flame::core
