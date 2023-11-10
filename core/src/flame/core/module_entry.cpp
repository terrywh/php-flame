#include "module_entry.h"
#include "exception.h"
#include <php/Zend/zend_modules.h>
#include <php/main/php.h>
#include <boost/assert.hpp>
#include <vector>
#include <cstring>

namespace flame::core {

class module_entry_finalizer {
    zend_module_entry entry_;
    std::vector<std::function<void ()>> start_;
    std::vector<std::function<void ()>> stop_;
    std::vector<zend_function_entry> fn_;
    std::vector<class_entry_base> class_;

    std::string name_;
    std::string version_;

    static module_entry_finalizer* ins_;
    static zend_result on_module_start(int type, int module) {
        std::set_terminate( flame::core::exception_handler );
        for (auto& ce : ins_->class_) ce.finalize();
        for (auto& fn : ins_->start_) fn();
        return ZEND_RESULT_CODE::SUCCESS;
    }

    static zend_result on_module_stop(int type, int module) {
        for (auto fn : ins_->stop_) fn();
        return ZEND_RESULT_CODE::SUCCESS;
    }
public:
    module_entry_finalizer(const std::string& name, const std::string& version)
    : entry_ {
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
        BOOST_ASSERT(entry_ == nullptr); // 仅允许初始化一个实例
        ins_ = this;
    }

    void* finalize() {
        fn_.push_back({});
        entry_.name = name_.data();
        entry_.version = version_.data();
        entry_.functions = fn_.data();
        return &entry_;
    }

    friend class module_entry;
}; // module_entry_finalizer

module_entry_finalizer* module_entry_finalizer::ins_ = nullptr;

module_entry::module_entry(const std::string& name, const std::string& version)
: entry_(std::make_unique<module_entry_finalizer>(name, version)) {
    
}

module_entry::~module_entry() = default;

module_entry& module_entry::operator +(on_module_start&& callback) {
    entry_->start_.push_back(callback.fn_);
    return *this;
}

module_entry& module_entry::operator +(on_module_stop&& callback) {
    entry_->stop_.push_back(callback.fn_);
    return *this;
}

module_entry& module_entry::operator +(function_entry&& entry) {
    entry.finalize(&entry_->fn_.emplace_back());
    return *this;
}

module_entry& module_entry::operator +(class_entry_base& entry) {
    entry_->class_.emplace_back(std::move(entry));
    return *this;
}

module_entry::operator void*() {
    return entry_->finalize();
}

} // flame::core
