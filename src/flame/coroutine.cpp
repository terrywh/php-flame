#include "controller.h"
#include "coroutine.h"
#include "../util.h"

static void coroutine_php_save_context(flame::coroutine::php_context_t &ctx) {
    ctx.vm_stack = EG(vm_stack);
    ctx.vm_stack_top = EG(vm_stack_top);
    ctx.vm_stack_end = EG(vm_stack_end);
    //   ctx.scope = EG(fake_scope);
    ctx.current_execute_data = EG(current_execute_data);
    ctx.exception = EG(exception);
    ctx.exception_class = EG(exception_class);
    ctx.error_handling = EG(error_handling);
}

static void coroutine_php_restore_context(flame::coroutine::php_context_t &ctx) {
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

// 参考 zend_execute.c
static void coroutine_php_vm_stack_init(void) {
    EG(current_execute_data) = nullptr;
    EG(error_handling) = EH_NORMAL;
    EG(exception_class) = nullptr;
    EG(exception) = nullptr;

    EG(vm_stack) = coroutine_php_vm_stack_new_page(4 * sizeof(zval) * 1024, NULL);
    EG(vm_stack)->top++;
    EG(vm_stack_top) = EG(vm_stack)->top;
    EG(vm_stack_end) = EG(vm_stack)->end;
}

namespace flame {

    unsigned int               coroutine::count = 0;
    std::shared_ptr<coroutine> coroutine::current;
    coroutine::php_context_t   coroutine::gctx_;

    void coroutine::start(php::callable fn) {
        ++coroutine::count;
        coroutine_php_save_context(coroutine::gctx_);
        auto co = std::make_shared<flame::coroutine>();
        
        boost::asio::post(co->ex_, [co, fn] () mutable {
            co->c1_ = boost::context::fiber([co, gd = boost::asio::make_work_guard(co->ex_), fn] (boost::context::fiber&& c2) mutable {
                co->c2_ = std::move(c2);
                
                coroutine_php_vm_stack_init();
                coroutine::current = co;
                
                fn.call(); // 产生 PHP 进栈 操作，才能进行捕获异常
                fn = nullptr;

                coroutine::current.reset();
                zend_vm_stack_destroy();
                coroutine_php_restore_context(coroutine::gctx_);
                
                --coroutine::count;
                
                return std::move(co->c2_);
            });
            co->c1_ = std::move(co->c1_).resume();
        });
    }

    void coroutine::suspend() {
        coroutine_php_save_context(cctx_);
        coroutine_php_restore_context(coroutine::gctx_);
        // coroutine::current.reset();
        c2_ = std::move(c2_).resume();
    }

    void coroutine::resume() {
        auto co = std::static_pointer_cast<coroutine>(shared_from_this());
        boost::asio::post(ex_, [co] () { 
            coroutine_php_save_context(coroutine::gctx_);
            coroutine_php_restore_context(co->cctx_);
            coroutine::current = co;
            co->c1_ = std::move(co->c1_).resume();
        });
    }

}
