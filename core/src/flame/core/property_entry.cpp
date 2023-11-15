#include "property_entry.h"
#include <php/Zend/zend_API.h>

namespace flame::core {

void property_entry::finalize(struct _zend_class_entry* ce) {
    std::uint32_t flag = acc_.finalize();
    if (flag == 0) flag |= ZEND_ACC_PUBLIC;
    // 参考 zend_declare_property_string() 传入的 property 值需要保留引用
    Z_TRY_ADDREF_P(value_.ptr());
    zend_declare_property(ce, name_.data(), name_.size(), value_, flag);
}

property_entry property(const std::string& name, const value& v) {
    return property_entry{name, v};
}

} // flame::core

