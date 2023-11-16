#include "method_entry.h"
#include "class_entry.h"
#include <php/Zend/zend_API.h>
#include <map>
#include <cstring>

namespace flame::core {

void* method_entry::fetch(struct _zend_execute_data *execute_data, int offset) { // z2c
    zend_object* obj = Z_OBJ_P(getThis());
    return reinterpret_cast<char*>(obj) - offset;
}

void method_entry::finalize(void *entry) {
    auto* e = reinterpret_cast<zend_function_entry*>(entry);
    zend_string* name = zend_string_init_interned(name_.data(), name_.size(), true); // 不释放（等待到进程退出）
    e->fname = ZSTR_VAL(name);
    // zend_string (interned) 由内部管理
    e->handler = fn_;
    e->arg_info = reinterpret_cast<zend_internal_arg_info*>(arg_.finalize());
    e->num_args = arg_.size();
    e->flags = acc_.finalize();
}

method_entry abstract_method(const std::string& name, access_entry acc) {
    return method_entry(name, nullptr, {value::type::null, {}}, acc % core::abstract_);
}

} // flame::core
