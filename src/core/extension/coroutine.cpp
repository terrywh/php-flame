#include "coroutine.h"
#include "../context.h"

namespace core { namespace extension {
    // 保存当前上下文，到参数容器中
    static void coroutine_php_save_context(coroutine::zend_context_t &ctx) {
        ctx.vm_stack = EG(vm_stack);
        ctx.vm_stack_top = EG(vm_stack_top);
        ctx.vm_stack_end = EG(vm_stack_end);
        //   ctx.scope = EG(fake_scope);
        ctx.current_execute_data = EG(current_execute_data);
        ctx.exception = EG(exception);
        ctx.exception_class = EG(exception_class);
        ctx.error_handling = EG(error_handling);
    }
    // 从参数容器中恢复上下文
    static void coroutine_php_restore_context(coroutine::zend_context_t &ctx) {
        EG(vm_stack) = ctx.vm_stack;
        EG(vm_stack_top) = ctx.vm_stack_top;
        EG(vm_stack_end) = ctx.vm_stack_end;
        // EG(fake_scope) = ctx.scope;
        EG(current_execute_data) = ctx.current_execute_data;
        EG(exception) = ctx.exception;
        EG(exception_class) = ctx.exception_class;
        EG(error_handling) = ctx.error_handling;
    }
    // 参考 zend_execute.c 
    static zend_vm_stack coroutine_php_vm_stack_new_page(size_t size, zend_vm_stack prev) {
        zend_vm_stack page = (zend_vm_stack)emalloc(size);

        page->top = ZEND_VM_STACK_ELEMENTS(page);
        page->end = (zval *)((char *)page + size);
        page->prev = prev;
        return page;
    }
    // 自定义的栈，为控制栈空间大小 php 64k + 32k
#define COROUTINE_PHP_STACK_SIZE 4 * sizeof(zval) * 1024 
#define COROUTINE_CPP_STACK_SIZE 32 * 1024 
    // 参考 zend_execute.c
    static void coroutine_php_vm_stack_init(void) {
        EG(current_execute_data) = nullptr;
        EG(error_handling) = EH_NORMAL;
        EG(exception_class) = nullptr;
        EG(exception) = nullptr;

        EG(vm_stack) = coroutine_php_vm_stack_new_page(COROUTINE_PHP_STACK_SIZE, NULL);
        EG(vm_stack)->top++;
        EG(vm_stack_top) = EG(vm_stack)->top;
        EG(vm_stack_end) = EG(vm_stack)->end;
    }

    unsigned short             coroutine::size_ = 0;
    std::shared_ptr<coroutine> coroutine::curr_;
    coroutine::zend_context_t  coroutine::gctx_;

    coroutine::coroutine()
    : basic_coroutine($context->io_m.get_executor()) {  }
    // 协程启动
    void coroutine::go(php::value fn) {
        coroutine_php_save_context(coroutine::gctx_);
        auto co = std::make_shared<coroutine>();
        
        boost::asio::post(co->ex_, [co, fn] () mutable {
            // 运行中的协程计数
            ++coroutine::size_;
            co->c1_ = boost::context::fiber(std::allocator_arg, boost::context::fixedsize_stack(COROUTINE_CPP_STACK_SIZE),
                [co, gd = boost::asio::make_work_guard(co->ex_), fn] (boost::context::fiber&& c2) mutable {
                    co->c2_ = std::move(c2);
                    
                    coroutine_php_vm_stack_init();
                    coroutine::curr_ = co;
                    
                    fn();
                    fn = nullptr;

                    coroutine::curr_.reset();
                    zend_vm_stack_destroy();
                    coroutine_php_restore_context(coroutine::gctx_);
                    
                    --coroutine::size_;
                    return std::move(co->c2_);
                });
            co->c1_ = std::move(co->c1_).resume();
        });
    }
    // 
    unsigned short coroutine::size() {
        return coroutine::size_;
    }
    //
    std::shared_ptr<coroutine> coroutine::current() {
        return curr_;
    }

    void coroutine::suspend() {
        coroutine_php_save_context(zctx_);
        coroutine_php_restore_context(coroutine::gctx_);
        // coroutine::current.reset();
        c2_ = std::move(c2_).resume();
    }

    void coroutine::resume() {
        boost::asio::post(ex_, [co = shared_from_this()] () { 
            coroutine_php_save_context(coroutine::gctx_);
            coroutine_php_restore_context(co->zctx_);
            coroutine::curr_ = co;
            co->c1_ = std::move(co->c1_).resume();
        });
    }

}}