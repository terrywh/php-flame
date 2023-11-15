#include "module_entry.h"
#include "constant_entry.h"
#include "function_entry.h"
#include "exception.h"
#include <php/Zend/zend_modules.h>
#include <php/main/php.h>
#include <boost/assert.hpp>
#include <vector>
#include <cstring>

namespace flame::core {

class module_entry_store {
    zend_module_entry entry;
    std::vector<std::function<void ()>> start;
    std::vector<std::function<void ()>>  stop;
    std::vector<zend_function_entry> function;
    std::vector<class_entry_base*>     class_;
    std::vector<constant_entry>      constant;

    std::string name_;
    std::string version_;

    static module_entry_store* ins_;
    static zend_result on_module_start(int type, int module) {
        std::set_terminate( flame::core::exception_handler );
        for (auto& cs : ins_->constant) cs.finalize(module);
        ins_->constant.clear();
        for (auto& ce : ins_->class_) ce->finalize();
        for (auto& fn : ins_->start) fn();
        ins_->start.clear();
        return ZEND_RESULT_CODE::SUCCESS;
    }

    static zend_result on_module_stop(int type, int module) {
        for (auto fn : ins_->stop) fn();
        ins_->stop.clear();
        return ZEND_RESULT_CODE::SUCCESS;
    }
public:
    module_entry_store(const std::string& name, const std::string& version)
    : entry {
        STANDARD_MODULE_HEADER_EX,
        nullptr, // INI ENTRY
        nullptr, // DEPENDENCIES
        nullptr, // MODULE NAME
        nullptr, // FUNCTION ENTRIES
        on_module_start,
        on_module_stop,
        nullptr, // REQUEST STARTUP
        nullptr, // REQUEST SHUTDOWN
        nullptr, // MODULE INFO
        nullptr, // VERSION
        STANDARD_MODULE_PROPERTIES,
    }
    , name_(name)
    , version_(version) {
        BOOST_ASSERT(entry == nullptr); // 仅允许初始化一个实例
        ins_ = this;
    }

    void* finalize() {
        function.push_back({});
        entry.name = name_.data();
        entry.version = version_.data();
        entry.functions = function.data();
        return &entry;
    }

    friend class module_entry;
}; // module_entry_finalizer

module_entry_store* module_entry_store::ins_ = nullptr;

module_entry::module_entry(const std::string& name, const std::string& version)
: store_(std::make_unique<module_entry_store>(name, version)) {
    
}

module_entry::~module_entry() = default;

module_entry& module_entry::operator +(on_module_start&& callback) {
    store_->start.push_back(callback.fn_);
    return *this;
}

module_entry& module_entry::operator +(on_module_stop&& callback) {
    store_->stop.push_back(callback.fn_);
    return *this;
}

module_entry& module_entry::operator +(function_entry&& entry) {
    entry.finalize(&store_->function.emplace_back());
    return *this;
}

module_entry& module_entry::operator +(constant_entry&& entry) {
    store_->constant.push_back(std::move(entry));
    return *this;
}

void module_entry::append_class_entry(class_entry_base& ce) {
    store_->class_.emplace_back(&ce);
}

module_entry::operator void*() {
    return store_->finalize();
}

} // flame::core
