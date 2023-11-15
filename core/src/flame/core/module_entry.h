#ifndef FLAME_PHP_ENTRY_MODULE_H
#define FLAME_PHP_ENTRY_MODULE_H
#include "class_entry.h"
#include <functional>
#include <memory>
#include <string>

namespace flame::core {

class on_module_start {
    std::function<void ()> fn_;
public:
    on_module_start(std::function<void ()> fn): fn_(fn) {}
    friend class module_entry;
};

class on_module_stop {
    std::function<void ()> fn_;
public:
    on_module_stop(std::function<void ()> fn): fn_(fn) {}
    friend class module_entry;
};

class function_entry;
class constant_entry;
class module_entry_store;

class module_entry {
    std::unique_ptr<module_entry_store> store_;
    void append_class_entry(class_entry_base& ce);

public:
    module_entry(const std::string& name, const std::string& version);
    ~module_entry();

    module_entry& operator +(on_module_start&& callback);
    module_entry& operator +(on_module_stop&& callback);
    module_entry& operator +(function_entry&& entry);
    module_entry& operator +(constant_entry&& entry);

    template <class T>
    class_entry_base& declare(const std::string& name) {
        class_entry_base& ce = class_entry_base::create(std::make_unique<class_entry_desc_basic<T>>(name));
        append_class_entry(ce);
        return ce;
    };

    // 返回 zend_module_entry* 结构指针（隐藏内部类型）
    operator void*();
};

} // flame::core

#endif // FLAME_PHP_ENTRY_MODULE_H
