#include "runtime.h"
#include <php/Zend/zend_API.h>
#include <php/Zend/zend_constants.h>

namespace flame::core {

runtime* runtime::ins_ = nullptr;

runtime::runtime() {
    ins_ = this;
    // TODO 可能的运行时初始化
}

value runtime::G(std::string_view name) const {
    return { zend_hash_str_find(&EG(symbol_table), name.data(), name.size()) };
}

value runtime::C(std::string_view name) const {
    return { zend_get_constant_str(name.data(), name.size()) };
}

} // flame::core
