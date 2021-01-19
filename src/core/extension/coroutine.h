#ifndef CORE_EXTENSION_COROUTINE_H
#define CORE_EXTENSION_COROUTINE_H

#include <phpext.h>
#include "../coroutine.h"
#include <memory>

namespace core { namespace extension {

    class coroutine : public basic_coroutine, public std::enable_shared_from_this<coroutine> {
    public:
        struct zend_context_t {
            zend_vm_stack vm_stack;
            zval *vm_stack_top;
            zval *vm_stack_end;
            zend_class_entry *scope;
            zend_execute_data *current_execute_data;

            zend_object *         exception;
            zend_error_handling_t error_handling;
            zend_class_entry *    exception_class;
        };

        static void go(php::value fn);
        static unsigned short size();
        static std::shared_ptr<coroutine> current();

        coroutine();
        void suspend();
        void resume();
    protected:
        zend_context_t zctx_;

        static zend_context_t             gctx_; // 当前上下文
        static unsigned short             size_; // 运行中的协程数量
        static std::shared_ptr<coroutine> curr_; // 当前协程
    };

    using coroutine_handler = basic_coroutine_handler<coroutine>;
    // class coroutine_handler: public core::coroutine_handler {
    // public:

    //     coroutine_handler()
    //     : core::coroutine_handler() {

    //     }

    //     coroutine_handler(const coroutine_handler& ch) = default;
    //     coroutine_handler(std::shared_ptr<coroutine> co)
    //     : core::coroutine_handler(co) {
            
    //     }

    //     ~coroutine_handler() {}

    //     void reset() {
    //         co_.reset();
    //     }

    //     void reset(std::shared_ptr<coroutine> co) {
    //         core::coroutine_handler::reset();
    //         co_ = co;
    //     }

    //     void reset(const coroutine_handler& ch) {
    //         core::coroutine_handler::reset();
    //         co_ = ch.co_;
    //     }

    //     coroutine_handler& operator[](boost::system::error_code& error) {
    //         error_ = &error;
    //         return *this;
    //     }
    //     coroutine_handler& operator[](std::size_t& count) {
    //         count_ = &count;
    //         return *this;
    //     }
    // };

}}

namespace boost::asio {
    template <>
    class async_result<::core::extension::coroutine_handler, void (boost::system::error_code error, std::size_t size)> {
    public:
        explicit async_result(::core::extension::coroutine_handler& ch) : ch_(ch), count_(0) {
            ch_.count_ = &count_;
        }
        using completion_handler_type = ::core::extension::coroutine_handler;
        using return_type = std::size_t;
        return_type get() {
            ch_.yield();
            return count_;
        }
    private:
        ::core::extension::coroutine_handler &ch_;
        std::size_t count_;
    };

    template <>
    class async_result<::core::extension::coroutine_handler, void (boost::system::error_code error)> {
    public:
        explicit async_result(::core::extension::coroutine_handler& ch) : ch_(ch) {
        }
        using completion_handler_type = ::core::extension::coroutine_handler;
        using return_type = void;
        void get() {
            ch_.yield();
        }
    private:
        ::core::extension::coroutine_handler &ch_;
    };
} // namespace boost::asio

#endif // CORE_EXTENSION_COROUTINE_H
