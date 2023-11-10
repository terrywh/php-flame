#ifndef FLAME_CORE_PARAMS_H
#define FLAME_CORE_PARAMS_H
#include "value.h"
#include <vector>
/**
 *  Forward declarations
 */
struct _zend_execute_data;
struct _zval_struct;

namespace flame::core {

class parameter_list {
    std::vector<flame::core::value> list_;

public:
    // 提取当前函数调用参数 (使用构造函数优先级低于 initializer_list 版本)
    static parameter_list prepare(struct _zend_execute_data* execute_data);
    parameter_list() {}
    parameter_list(std::initializer_list<value> list);
    //
    inline std::size_t size() const {
        return list_.size();
    }
    inline auto begin() {
        return list_.begin();
    }
    inline auto end() {
        return list_.end();
    }
    inline value& operator [](int index) {
        return list_[index];
    }
    // 检查调用参数与参数声明是否匹配，若不匹配抛出错误并返回 false 否则返回 true
    bool verify() const;
};

} // flame::core

#endif // FLAME_CORE_PARAMS_H
