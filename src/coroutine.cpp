#include "controller.h"
#include "coroutine.h"

namespace flame
{
    void coroutine::save_context(php_context_t &ctx)
    {
        ctx.vm_stack = EG(vm_stack);
        ctx.vm_stack_top = EG(vm_stack_top);
        ctx.vm_stack_end = EG(vm_stack_end);
        //   ctx.scope = EG(fake_scope);
        ctx.current_execute_data = EG(current_execute_data);
    }
    void coroutine::restore_context(php_context_t &ctx)
    {
        EG(vm_stack) = ctx.vm_stack;
        EG(vm_stack_top) = ctx.vm_stack_top;
        EG(vm_stack_end) = ctx.vm_stack_end;
        // EG(fake_scope) = ctx.scope;
        EG(current_execute_data) = ctx.current_execute_data;
    }
    void coroutine::start(php::callable fn, zend_execute_data* execute_data)
    {
        auto co = std::make_shared<coroutine>(std::move(fn));
        // 需要即时保存 PHP 堆栈
        co->php_.current_execute_data = execute_data == nullptr ? EG(current_execute_data) : execute_data;
        // 事实上的协程启动会稍后
        boost::asio::post(gcontroller->context_x, [co] {
            // co->c1_ = boost::context::callcc([co] (auto &&cc) {
            co->c1_ = boost::context::fiber([co] (boost::context::fiber &&cc) {
                auto work = boost::asio::make_work_guard(gcontroller->context_x);
                // 启动进入协程
                coroutine::current = co.get();
                zend_vm_stack_init();
                co->c2_ = std::move(cc);
                // 协程运行
                co->fn_.call();
                // 协程运行完毕
                zend_vm_stack_destroy();
                coroutine::current = nullptr;
                return std::move(co->c2_);
            });
            co->resume();
        });
    }
    coroutine::coroutine(php::callable &&fn)
        : fn_(std::move(fn)), c1_(), c2_() {}

    void coroutine::suspend()
    {
        // 保存 PHP 堆栈
        coroutine::save_context(php_);
        // 离开协程
        coroutine::current = nullptr;
        c2_ = std::move(c2_).resume();
    }

    void coroutine::resume()
    {
        // 恢复 PHP 堆栈
        coroutine::restore_context(php_);
        // 恢复进入协程
        coroutine::current = this;
        c1_ = std::move(c1_).resume();
    }

    coroutine_handler::coroutine_handler(coroutine *co)
        : co_(co)
    {
    }
    coroutine_handler::~coroutine_handler()
    {
        // std::cout << "~coroutine_handler\n";
    }
    void coroutine_handler::operator()(const boost::system::error_code &e, std::size_t n)
    {
        error = e;
        nsize = n;

        assert(std::this_thread::get_id() == gcontroller->mthread_id);
        co_->resume();
    }
    void coroutine_handler::resume()
    {
        assert(std::this_thread::get_id() == gcontroller->mthread_id);
        co_->resume();
    }
    void coroutine_handler::suspend()
    {
        assert(std::this_thread::get_id() == gcontroller->mthread_id);
        co_->suspend();
    }
}