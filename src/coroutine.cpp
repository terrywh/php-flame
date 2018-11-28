#include "controller.h"
#include "coroutine.h"

namespace flame
{
    // 当前协程
    std::shared_ptr<coroutine> coroutine::current;
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
    std::shared_ptr<coroutine> coroutine::start(php::callable fn, /*std::vector<php::value> ag,*/ zend_execute_data *execute_data)
    {
        auto co = std::make_shared<coroutine>(std::move(fn));
        // 需要即时保存 PHP 堆栈
        co->php_.current_execute_data = execute_data;
        // 事实上的协程启动会稍后
        boost::asio::post(gcontroller->context_x, [co/*, ag = std::move(ag)*/] {
            // co->c1_ = boost::context::callcc([co] (auto &&cc) {
            co->c1_ = boost::context::fiber([co/*, ag = std::move(ag)*/] (boost::context::fiber &&cc) {
                auto work = boost::asio::make_work_guard(gcontroller->context_x);
                // 启动进入协程
                coroutine::current = co;
                if(co->php_.current_execute_data == nullptr)
                {
                    // 若 run 还未执行时, core_execute_data 还没有值;
                    // 故赋值延迟到此处进行
                    co->php_.current_execute_data = gcontroller->core_execute_data;
                }
                zend_vm_stack_init();
                co->c2_ = std::move(cc);
                // 协程运行
                co->fn_.call(/*ag*/);
                // 协程运行完毕
                zend_vm_stack_destroy();
                coroutine::current = nullptr;
                return std::move(co->c2_);
            });
            co->resume();
        });
        return co;
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
        coroutine::current = shared_from_this();
        c1_ = std::move(c1_).resume();
    }
    coroutine_handler::coroutine_handler()
        : co_(nullptr)
        , er_(nullptr)
    {
    }
    coroutine_handler::coroutine_handler(std::shared_ptr<coroutine> co)
        : co_(co)
        , er_(nullptr)
    {
    }
    coroutine_handler::~coroutine_handler()
    {
        // std::cout << "~coroutine_handler\n";
    }
    void coroutine_handler::reset()
    {
        co_.reset();
    }
    void coroutine_handler::reset(std::shared_ptr<coroutine> co)
    {
        assert(co_ == nullptr);
        co_ = co;
    }
    coroutine_handler::operator bool() const
    {
        return co_ != nullptr;
    }
    void coroutine_handler::operator() (const boost::system::error_code &e, std::size_t n)
    {
        if(er_) *er_ = e;
        co_->len_ = n;

        assert(std::this_thread::get_id() == gcontroller->mthread_id);
        boost::asio::post(gcontroller->context_x, std::bind(&coroutine::resume, co_));
    }
    coroutine_handler& coroutine_handler::operator [](boost::system::error_code& e)
    {
        er_ = &e;
        return *this;
    }
    void coroutine_handler::resume()
    {
        assert(std::this_thread::get_id() == gcontroller->mthread_id);
        boost::asio::post(gcontroller->context_x, std::bind(&coroutine::resume, co_));
    }
    void coroutine_handler::suspend()
    {
        assert(std::this_thread::get_id() == gcontroller->mthread_id);
        co_->suspend();
    }
    bool operator<(const coroutine_handler &ch1, const coroutine_handler &ch2)
    {
        return ch1.co_ < ch2.co_;
    }
}