#pragma once
#include "../vendor.h"
#include "../coroutine.h"
#include "controller.h"

namespace flame {

    class coroutine : public ::coroutine {
    public:
        struct php_context_t {
            zend_vm_stack vm_stack;
            zval *vm_stack_top;
            zval *vm_stack_end;
            zend_class_entry *scope;
            zend_execute_data *current_execute_data;

            zend_object *         exception;
            zend_error_handling_t error_handling;
            zend_class_entry *    exception_class;
        };
        static void start(php::callable fn);
        static unsigned int count;
        static std::shared_ptr<flame::coroutine> current;

        coroutine()
        : ::coroutine(gcontroller->context_x.get_executor()) {

        }
        void suspend();
        void resume();
    protected:
        static php_context_t gctx_;
               php_context_t cctx_;

        friend class coroutine_handler;
    };

    class coroutine_handler: public ::coroutine_handler {
    public:
        coroutine_handler()
        : ::coroutine_handler() {

        }
        coroutine_handler(const coroutine_handler& ch) = default;
        coroutine_handler(std::shared_ptr<coroutine> co)
        : ::coroutine_handler(co) {
            
        }
        ~coroutine_handler() {

        }

        void reset() {
            co_.reset();
        }
        void reset(std::shared_ptr<coroutine> co) {
            co_ = co;
        }
        void reset(const coroutine_handler& ch) {
            co_ = ch.co_;
        }

        coroutine_handler& operator[](boost::system::error_code& error) {
            error_ = &error;
            return *this;
        }
        coroutine_handler& operator[](std::size_t& size) {
            size_ = &size;
            return *this;
        }
    };

}

namespace boost::asio {
    template <>
    class async_result<flame::coroutine_handler, void (boost::system::error_code error, std::size_t size)> {
    public:
        explicit async_result(flame::coroutine_handler& ch) : ch_(ch), size_(0) {
            ch_.operator[](size_);
        }
        using completion_handler_type = flame::coroutine_handler;
        using return_type = std::size_t;
        return_type get() {
            ch_.suspend();
            return size_;
        }
    private:
        flame::coroutine_handler &ch_;
        std::size_t size_;
    };

    template <>
    class async_result<flame::coroutine_handler, void (boost::system::error_code error)> {
    public:
        explicit async_result(flame::coroutine_handler& ch) : ch_(ch) {
        }
        using completion_handler_type = flame::coroutine_handler;
        using return_type = void;
        void get() {
            ch_.suspend();
        }
    private:
        flame::coroutine_handler &ch_;
    };
} // namespace boost::asio
