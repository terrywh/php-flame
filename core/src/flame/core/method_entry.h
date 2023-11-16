#ifndef FLAME_CORE_METHOD_ENTRY_H
#define FLAME_CORE_METHOD_ENTRY_H
#include "access_entry.h"
#include "argument_entry.h"
#include "function_entry.h"
#include "class_entry_desc.h"
#include <memory>

struct _zval_struct;
struct _zend_execute_data;
using zend_function_ptr = void(*)(struct _zend_execute_data *execute_data, struct _zval_struct *return_value);

namespace flame::core {

class module_entry;

class method_entry {
    std::string         name_;
    zend_function_ptr     fn_;
    argument_entry       arg_;
    access_entry         acc_;

public:
    method_entry(const std::string& name, zend_function_ptr fn, argument_entry&& arg, access_entry acc)
    : name_(name)
    , fn_(fn)
    , arg_(std::move(arg))
    , acc_(acc) {}

    method_entry& operator %(access_entry::modifier m) & {
        acc_ % m;
        return *this;
    }
    method_entry&& operator %(access_entry::modifier m) && {
        acc_ % m;
        return std::move(*this);
    }

    static void* fetch(struct _zend_execute_data *execute_data, int offset);

    void finalize(void *entry);

    template <class T, void (T::*F)()>
    static void handler(struct _zend_execute_data *execute_data, struct _zval_struct *return_value) {
        (reinterpret_cast<T*>( fetch(execute_data, class_entry_desc_basic<T>::size()) )->*F)();
        function_entry::do_return(return_value);
    }
    template <class T, void (T::*F)(flame::core::parameter_list& list)>
    static void handler(struct _zend_execute_data *execute_data, struct _zval_struct *return_value) {
        auto argv = flame::core::parameter_list::prepare(execute_data);
        if (!function_entry::do_verify(execute_data)) return;
        (reinterpret_cast<T*>( fetch(execute_data, class_entry_desc_basic<T>::size()) )->*F)(argv);
        function_entry::do_return(return_value);
    }
    template <class T, flame::core::value (T::*F)()>
    static void handler(struct _zend_execute_data *execute_data, struct _zval_struct *return_value) {
        function_entry::do_return(execute_data, return_value,
            (reinterpret_cast<T*>( fetch(execute_data, class_entry_desc_basic<T>::size()) )->*F)());
    }
    template <class T, flame::core::value (T::*F)(flame::core::parameter_list& list)>
    static void handler(struct _zend_execute_data *execute_data, struct _zval_struct *return_value) {
        auto argv = flame::core::parameter_list::prepare(execute_data);
        if (!function_entry::do_verify(execute_data)) return;
        function_entry::do_return(execute_data, return_value,
            (reinterpret_cast<T*>( fetch(execute_data, class_entry_desc_basic<T>::size()) )->*F)(argv));
    }
};

template <auto>
class method;

template <class T, void (T::*F)()>
class method<F> {
    method_entry entry_;
public:
    explicit method(const std::string& name)
    : entry_{ name, method_entry::handler<T, F>, {type::null, {}}, {} } {}
    operator method_entry&&() { return std::move(entry_); }
};

template <class T, void (T::*F)(flame::core::parameter_list& list)>
class method<F> {
    method_entry entry_;
public:
    explicit method(const std::string& name, std::initializer_list<argument_desc> list)
    : entry_ { name, method_entry::handler<T, F>, {type::null, std::move(list)}, {} } {}
    operator method_entry&&() { return std::move(entry_); }
};

template <class T, flame::core::value (T::*F)()>
class method<F> {
    method_entry entry_;
public:
    explicit method(const std::string& name, argument_desc::type type)
    : entry_ { name, method_entry::handler<T, F>, {type, {}}, {} } {}
    operator method_entry&&() { return std::move(entry_); }
};

template <class T, flame::core::value (T::*F)(flame::core::parameter_list& list)>
class method<F> {
    method_entry entry_;
public:
    explicit method(const std::string& name, argument_desc::type type, std::initializer_list<argument_desc> list)
    : entry_ { name, method_entry::handler<T, F>, {type, std::move(list)}, {} } {}
    operator method_entry&&() { return std::move(entry_); }
};

method_entry abstract_method(const std::string& name, access_entry acc);

} // flame::core

#endif // FLAME_CORE_METHOD_ENTRY_H