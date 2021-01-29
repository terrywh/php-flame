#ifndef CORE_EXTENSION_COROUTINE_H
#define CORE_EXTENSION_COROUTINE_H

#include <phpext.h>
#include "../coroutine.h"
#include <memory>

namespace core { namespace extension {

// 自定义的栈，为控制栈空间大小 php 64k + 32k
#define COROUTINE_PHP_STACK_SIZE 4 * sizeof(zval) * 1024 
#define COROUTINE_CPP_STACK_SIZE 32 * 1024 

    struct coroutine_context {
        zend_vm_stack vm_stack;
        zval *vm_stack_top;
        zval *vm_stack_end;
        zend_class_entry *scope;
        zend_execute_data *current_execute_data;

        zend_object *         exception;
        zend_error_handling_t error_handling;
        zend_class_entry *    exception_class;
    };
    // 
    class coroutine_traits {
    public:
        static unsigned int      cnt_; // 活跃协程数量
        static coroutine_context gtx_; // 全局运行状态，用于进入退出
               coroutine_context ctx_; // 当前协程运行状态，用于恢复
    
        // 初始化上下文
        static void init_context(coroutine_context &ctx);
        // 将当前上下文保存到参数容器中
        static void save_context(coroutine_context &ctx);
        // 从参数容器中恢复上下文
        static void restore_context(coroutine_context &ctx);
    public:
        void  start();
        void  yield();
        void resume();
        void    end();
    };

    using coroutine = basic_coroutine<coroutine_traits>;
    using coroutine_handler = basic_coroutine_handler<coroutine>;
}}


namespace boost::asio {
    template <>
    class async_result<::core::extension::coroutine_handler, void (boost::system::error_code error, std::size_t size)> {
    public:
        explicit async_result(::core::extension::coroutine_handler& ch) : ch_(ch), size_(0) {
            ch_.count_ = &size_;
        }
        using completion_handler_type = ::core::extension::coroutine_handler;
        using return_type = std::size_t;
        return_type get() {
            ch_.yield();
            return size_;
        }
    private:
        ::core::extension::coroutine_handler &ch_;
        std::size_t size_;
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
