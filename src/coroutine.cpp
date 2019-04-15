#include "controller.h"
#include "coroutine.h"

namespace flame {

    boost::context::fixedsize_stack coroutine::stack_allocator(64 * 1024);

    std::size_t coroutine::count = 0;

    coroutine::php_context_t coroutine::global_context;
    // 当前协程
    std::shared_ptr<coroutine> coroutine::current(nullptr);

    void coroutine::save_context(php_context_t &ctx) {
        ctx.vm_stack = EG(vm_stack);
        ctx.vm_stack_top = EG(vm_stack_top);
        ctx.vm_stack_end = EG(vm_stack_end);
        //   ctx.scope = EG(fake_scope);
        ctx.current_execute_data = EG(current_execute_data);
        ctx.exception = EG(exception);
        ctx.exception_class = EG(exception_class);
        ctx.error_handling = EG(error_handling);
    }

    void coroutine::restore_context(php_context_t &ctx) {
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
    static zend_vm_stack zend_vm_stack_new_page(size_t size, zend_vm_stack prev) {
        zend_vm_stack page = (zend_vm_stack)emalloc(size);

        page->top = ZEND_VM_STACK_ELEMENTS(page);
        page->end = (zval *)((char *)page + size);
        page->prev = prev;
        return page;
    }
    // 参考 zend_execute.c
    static void zend_vm_stack_init(void) {
        EG(vm_stack) = zend_vm_stack_new_page(4 * sizeof(zval) * 1024, NULL);
        EG(vm_stack)->top++;
        EG(vm_stack_top) = EG(vm_stack)->top;
        EG(vm_stack_end) = EG(vm_stack)->end;
    }

    std::shared_ptr<coroutine> coroutine::start(php::callable fn) {
        ++coroutine::count;
        auto co_ = std::make_shared<coroutine>(fn);
        // 事实上的协程启动会稍后
        boost::asio::post(gcontroller->context_x, [co_ /*, ag = std::move(ag)*/] {
            co_->c1_ = boost::context::fiber(
                std::allocator_arg, coroutine::stack_allocator,
                [co_ /*, ag = std::move(ag)*/](boost::context::fiber &&cc) {

                co_->c2_ = std::move(cc);
                auto work = boost::asio::make_work_guard(gcontroller->context_x);

                save_context(coroutine::global_context);
                // 启动进入协程
                coroutine::current = co_;

                EG(current_execute_data) = nullptr;
                EG(error_handling) = EH_NORMAL;
                EG(exception_class) = nullptr;
                EG(exception) = nullptr;
                zend_vm_stack_init();
                // 协程运行;
                co_->fn_.call(/*ag*/);
                // 实际协程销毁可能会较晚, 保证在 Zend 引擎前释放
                co_->fn_ = nullptr;
                // 协程运行完毕
                zend_vm_stack_destroy();
                coroutine::current = nullptr;
                
                restore_context(coroutine::global_context);
                boost::asio::post(gcontroller->context_x, [co_]() {
                    co_->c1_ = boost::context::fiber();
                    if (--coroutine::count == 0) gcontroller->stop(); // 所有协程结束后退出
                });
                return std::move(co_->c2_);
            });

            co_->c1_ = std::move(co_->c1_).resume();
        });
        return co_;
    }

    coroutine::coroutine(php::callable fn)
    : fn_(std::move(fn)), c1_(), c2_() {
        // std::cout << "coroutine\n";
    }

    coroutine::~coroutine() {
        // std::cout << "~coroutine\n";
    }

    void coroutine::suspend() {
        // 保存 PHP 堆栈
        coroutine::save_context(php_);
        coroutine::restore_context(coroutine::global_context);
        // 离开协程
        coroutine::current = nullptr;
        c2_ = std::move(c2_).resume();
    }

    void coroutine::resume() {
        // 恢复 PHP 堆栈
        coroutine::save_context(coroutine::global_context);
        coroutine::restore_context(php_);
        // 恢复进入协程
        coroutine::current = shared_from_this();
        c1_ = std::move(c1_).resume();
    }
    coroutine_handler::coroutine_handler()
    : co_(nullptr)
    , err_(nullptr)
    , stat_(new std::atomic<int>(0)) {
    }

    coroutine_handler::coroutine_handler(std::shared_ptr<coroutine> co)
    : co_(co)
    , err_(nullptr)
    , stat_(new std::atomic<int>(0)) {
    }

    // coroutine_handler::coroutine_handler(coroutine_handler&& ch)
    // : co_(std::move(ch.co_))
    // , err_(ch.err_)
    // , stat_(std::move(ch.stat_)) {
    //     ch.err_ = nullptr;
    // }

    coroutine_handler::~coroutine_handler() {
        // std::cout << "~coroutine_handler\n";
    }

    void coroutine_handler::reset() {
        co_.reset();
        *stat_ = 0;
    }

    void coroutine_handler::reset(std::shared_ptr<coroutine> co) {
        assert(co_ == nullptr);
        co_   = co;
        *stat_ = 0;
    }

    coroutine_handler::operator bool() const {
        return co_ != nullptr;
    }

    void coroutine_handler::operator() (const boost::system::error_code &e, std::size_t n) {
        if (err_) *err_ = e;
        co_->len_ = n;
        resume();
    }

    void coroutine_handler::operator() (const boost::system::error_code &e) {
        if (err_) *err_ = e;
        // co_->len_ = 0;
        resume();
    }

    coroutine_handler& coroutine_handler::operator [](boost::system::error_code& e) {
        err_ = &e;
        return *this;
    }

    void coroutine_handler::resume() {
        if (--*stat_ == 0) boost::asio::post(gcontroller->context_x, std::bind(&coroutine::resume, co_));
    }

    void coroutine_handler::suspend() {
        assert(std::this_thread::get_id() == gcontroller->mthread_id);
        if (++*stat_ == 1) co_->suspend();
    }
    
    bool operator<(const coroutine_handler &ch1, const coroutine_handler &ch2) {
        return ch1.co_ < ch2.co_;
    }
}
