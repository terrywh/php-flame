#include "argument_entry.h"
#include <php/Zend/zend_API.h>
#include <memory>
#include <vector>
#include <cstring>

namespace flame::core {

static const std::uint32_t CODE_BY_REFERENCE = 0x100;
static const std::uint32_t CODE_IS_VARIADIC  = 0x200;
static const std::uint32_t CODE_ALLOW_NULL   = 0x400;

argument_desc::modifier argument_desc::by_reference { CODE_BY_REFERENCE };
argument_desc::modifier argument_desc::is_variadic  { CODE_IS_VARIADIC };
argument_desc::modifier argument_desc::allow_null   { CODE_ALLOW_NULL };

void argument_desc::type::finalize(void* entry) const {
    auto* e = reinterpret_cast<zend_type*>(entry);
    if (name_.empty()) { // 内部类型
        *e = zend_type ZEND_TYPE_INIT_CODE(code_, (flag_ & CODE_ALLOW_NULL) ? 1 : 0,
            _ZEND_ARG_INFO_FLAGS( (flag_ & CODE_BY_REFERENCE)?1:0, (flag_ & CODE_IS_VARIADIC)?1:0, 0));
    } else { // 类类型（名称）
        // 实际名称 name_.data() 会在 zend_register_functions 中进行 zend_string_init_interned 复制
        *e = zend_type ZEND_TYPE_INIT_CLASS(name_.data(), (flag_ & CODE_ALLOW_NULL) ? 1 : 0,
            _ZEND_ARG_INFO_FLAGS( (flag_ & CODE_BY_REFERENCE)?1:0, (flag_ & CODE_IS_VARIADIC)?1:0, 0));
    }
}

void argument_desc::finalize(void * entry) const {
    auto* e = reinterpret_cast<zend_internal_arg_info*>(entry);
    zend_string* name = zend_string_init_interned(name_.data(), name_.size(), 1);
    e->name = ZSTR_VAL(name);
    // zend_string (interned) 由内部管理
    type_.finalize(&e->type);
    e->default_value = nullptr;
}

argument_desc byref(const std::string& name, argument_desc::type type) {
    return argument_desc{name, type, CODE_BY_REFERENCE};
}

argument_desc byval(const std::string& name, argument_desc::type type) {
    return argument_desc{name, type};
}

} // flame::core
