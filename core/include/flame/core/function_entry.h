#ifndef FLAME_CORE_ENTRY_FUNCTION_H
#define FLAME_CORE_ENTRY_FUNCTION_H
#include "value.h"
#include "parameter.h"
#include "argument_entry.h"
#include "access_entry.h"
#include <rome/delegate.hpp>
#include <iostream>
#include <memory>
#include <string>

/**
 *  Forward declarations
 */
struct _zend_execute_data;
struct _zval_struct;

namespace flame::core {

using zend_function_ptr = void(*)(struct _zend_execute_data *execute_data, struct _zval_struct *return_value);

class function_entry {
    std::string         name_;
    zend_function_ptr     fn_;
    argument_entry_list arg_;
    access_entry        acc_;
public:
    function_entry(const std::string& name, zend_function_ptr fn, argument_entry_list arg, access_entry acc)
    : name_(name)
    , fn_(fn)
    , arg_(std::move(arg))
    , acc_(std::move(acc)) { }

    // function_entry(const function_entry& entry) = delete;
    function_entry(function_entry&& entry) = default;

    // 填充 entry 数据（隐藏 zend_function_entry 结构定义）
    void finalize(void * entry);

    static bool do_verify(struct _zend_execute_data *execute_data);
    static void do_return(struct _zval_struct *return_value);
    static void do_return(struct _zend_execute_data *execute_data, struct _zval_struct *return_value, const flame::core::value& v);

    template <void (*F)()>
    static void handler(struct _zend_execute_data *execute_data, struct _zval_struct *return_value) {
        F();
        do_return(return_value);
    }
    template <void (*F)(flame::core::parameter_list& list)>
    static void handler(struct _zend_execute_data *execute_data, struct _zval_struct *return_value) {
        auto argv = flame::core::parameter_list::prepare(execute_data);
        if (!do_verify(execute_data)) return;
        F(argv);
        do_return(return_value);
    }
    template <flame::core::value (*F)()>
    static void handler(struct _zend_execute_data *execute_data, struct _zval_struct *return_value) {
        flame::core::value r = F();
        do_return(execute_data, return_value, r);
    }
    template <flame::core::value (*F)(flame::core::parameter_list& list)>
    static void handler(struct _zend_execute_data *execute_data, struct _zval_struct *return_value) {
        auto argv = flame::core::parameter_list::prepare(execute_data);
        if (!do_verify(execute_data)) return;
        do_return(execute_data, return_value, F(argv));
    }

}; // function_entry

template <void (*F)()>
function_entry function(const std::string& name) {
    return function_entry(name, function_entry::handler<F>, {value::type::null, {}}, {});
}
template <void (*F)(flame::core::parameter_list& list)>
function_entry function(const std::string& name, std::initializer_list<argument_entry> list) {
    return function_entry(name, function_entry::handler<F>, {value::type::null, std::move(list)}, {});
}
template <flame::core::value (*F)()>
function_entry function(const std::string& name, argument::type type) {
    return function_entry(name, function_entry::handler<F>, {type, {}}, {});
}
template <flame::core::value (*F)(flame::core::parameter_list& list)>
function_entry function(const std::string& name, argument::type type, std::initializer_list<argument_entry> list) {
    return function_entry(name, function_entry::handler<F>, {type, std::move(list)}, {});
}
} // flame::core

#endif // FLAME_CORE_ENTRY_FUNCTION_H
