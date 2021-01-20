#include "coroutine.h"
#include "../context.h"

namespace core { namespace extension {
    // 参考 zend_execute.c 
    static zend_vm_stack vm_stack_new_page(size_t size, zend_vm_stack prev) {
        zend_vm_stack page = (zend_vm_stack)emalloc(size);

        page->top = ZEND_VM_STACK_ELEMENTS(page);
        page->end = (zval *)((char *)page + size);
        page->prev = prev;
        return page;
    }
    // 参考 zend_execute.c
    void coroutine_traits::init_context(coroutine_context &ctx) {
        ctx.vm_stack = vm_stack_new_page(COROUTINE_PHP_STACK_SIZE, NULL);
        ctx.vm_stack->top++;
        ctx.vm_stack_top = ctx.vm_stack->top;
        ctx.vm_stack_end = ctx.vm_stack->end;

        ctx.current_execute_data = nullptr;
        ctx.error_handling = EH_NORMAL;
        ctx.exception_class = nullptr;
        ctx.exception = nullptr;
    }
    // 将当前上下文保存到参数容器中
    void coroutine_traits::save_context(coroutine_context &ctx) {
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
    void coroutine_traits::restore_context(coroutine_context &ctx) {
        EG(vm_stack) = ctx.vm_stack;
        EG(vm_stack_top) = ctx.vm_stack_top;
        EG(vm_stack_end) = ctx.vm_stack_end;
        // EG(fake_scope) = ctx.scope;
        EG(current_execute_data) = ctx.current_execute_data;
        EG(exception) = ctx.exception;
        EG(exception_class) = ctx.exception_class;
        EG(error_handling) = ctx.error_handling;
    }

    coroutine_traits::coroutine_traits() {
        save_context(gtx_);
    }

    void coroutine_traits::start() {
        ++cnt_;
        init_context(ctx_);
        restore_context(ctx_);
    }

    void coroutine_traits::end() {
        --cnt_;
        zend_vm_stack_destroy(); // 相当于对 ctx_ 操作
        restore_context(gtx_);
    }

    void coroutine_traits::yield() {
        save_context(ctx_);
        restore_context(gtx_);
    }

    void coroutine_traits::resume() {
        save_context(gtx_);
        restore_context(ctx_);
    }
}}