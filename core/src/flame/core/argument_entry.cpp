#include "argument_entry.h"
#include <php/Zend/zend_API.h>
#include <memory>
#include <vector>
#include <cstring>

namespace flame::core {

static const std::uint32_t CODE_BY_REFERENCE = 0x100;
static const std::uint32_t CODE_IS_VARIADIC  = 0x200;
static const std::uint32_t CODE_ALLOW_NULL   = 0x400;

argument::modifier argument::by_reference { CODE_BY_REFERENCE };
argument::modifier argument::is_variadic  { CODE_IS_VARIADIC };
argument::modifier argument::allow_null   { CODE_ALLOW_NULL };

void argument::type::finalize(void* entry) const {
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

void argument_entry::finalize(void * entry) const {
    auto* e = reinterpret_cast<zend_internal_arg_info*>(entry);
    e->name = name_.data();
    type_.finalize(&e->type);
    e->default_value = nullptr;
}

argument_entry byref(const std::string& name, argument::type type) {
    return argument_entry{name, type, CODE_BY_REFERENCE};
}

argument_entry byval(const std::string& name, argument::type type) {
    return argument_entry{name, type};
}

argument_entry_list::argument_entry_list(argument::type type, std::initializer_list<argument_entry> list)
: type_(type)
, list_(std::move(list)) {

}

std::size_t argument_entry_list::size() const {
    return list_.size(); // 首个元素描述必要参数数量
}

static std::vector< std::unique_ptr<std::vector<_zend_internal_arg_info>> > argument_entry_registry;

void* argument_entry_list::finalize() {
    auto v = std::make_unique<std::vector<_zend_internal_arg_info>>();
    v->reserve(list_.size() + 1); // 保证在创建过程中不进行内存重新分配

    auto *rv = reinterpret_cast<zend_internal_function_info*>(&v->emplace_back());
    std::memset(rv, 0, sizeof(zend_internal_function_info));
    type_.finalize(&rv->type);

    for (auto& item : list_) {
        auto& e = v->emplace_back();
        item.finalize(&e);
        if (!ZEND_TYPE_ALLOW_NULL(e.type)) {
            ++rv->required_num_args;
        }
    }

    return argument_entry_registry.emplace_back(std::move(v))->data();
}

} // flame::core
