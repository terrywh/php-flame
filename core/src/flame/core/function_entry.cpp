#include "function_entry.h"
#include <php/Zend/zend_API.h>
#include <iostream>
#include <vector>

namespace flame::core {

void function_entry::finalize(void* entry) {
    auto* e = reinterpret_cast<zend_function_entry*>(entry);
    zend_string* name = zend_string_init_interned(name_.data(), name_.size(), true);
    e->fname = name->val;
    e->handler = fn_;
    e->arg_info = reinterpret_cast<zend_internal_arg_info*>(arg_.finalize());
    e->num_args = arg_.size();
    e->flags = 0;
}

bool function_entry::do_verify(struct _zend_execute_data *execute_data) {
    // 由于 PHP 内置的检查过程 zend_internal_call_should_throw 函数未导出（且仅在 ZEND_DEBUG 下启用）
    // 在实际参数访问时对类型进行 ASSERT 检查
    if (execute_data->func->common.required_num_args > ZEND_CALL_NUM_ARGS(execute_data)) {
        zend_missing_arg_error(execute_data);
        return false;
    }
    return true;
    // if (!(call_->func->common.fn_flags & ZEND_ACC_HAS_TYPE_HINTS)) {
    //     return true;
    // }

}

void function_entry::do_return(struct _zval_struct *return_value) {
    RETVAL_NULL();
}

void function_entry::do_return(struct _zend_execute_data *execute_data, struct _zval_struct *return_value, const flame::core::value& v) {
    RETVAL_ZVAL(v, 1, 0);
}

} // flame::core
