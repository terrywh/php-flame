#ifndef FLAME_PHP_ENTRY_MODULE_H
#define FLAME_PHP_ENTRY_MODULE_H
#include "function_entry.h"
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

class module_entry_finalizer;

class module_entry {
    std::unique_ptr<module_entry_finalizer> entry_;

public:
    module_entry(const std::string& name, const std::string& version);
    ~module_entry();

    module_entry& operator +(on_module_start&& callback);
    module_entry& operator +(on_module_stop&& callback);

    module_entry& operator +(function_entry&& entry);
    module_entry& operator +(class_entry_base& entry);

    // 返回 zend_module_entry* 结构指针（隐藏内部类型）
    operator void*();
};

} // flame::core

#endif // FLAME_PHP_ENTRY_MODULE_H
