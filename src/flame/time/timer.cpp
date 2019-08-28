#include "time.h"
#include "timer.h"
#include "scheduler.h"

namespace flame::time {
    timer_base::timer_base(std::size_t ms, php::callable cb, bool repeat)
    : interval_(ms)
    , cb_(cb)
    , resolv_(0)
    , repeat_(repeat) {
        // std::cout << " timer_base: " << this << "\n";
        reschedule();
    }

    timer_base::~timer_base() {
        // std::cout << "~timer_base: " << this << "\n";
    }

    void timer_base::start() {
        coroutine::start(php::callable([this] (php::parameters& params) -> php::value {
            coroutine_handler ch {coroutine::current};
            php::value rv;
            do {
                // 注意：定时器 TIMER 触发时可能当前协程未处于时间等待状态（其他异步流程）
                if(resolv_ == 0) {
                    ch_ = ch;
                    ch.suspend();
                    ch_.reset();
                }
                if(resolv_ < 1) break;
                --resolv_; // 由于其他异步或阻塞流程，导致实际 TIMER 可能已经触发了多次
                rv = cb_.call();
                if(rv.type_of(php::TYPE::NO)) repeat_ = false;
            }
            while(repeat_);

            resolv_ = -1; // 结束标记
            return nullptr;
        }));

    }

    void timer_base::close() {
        repeat_ = false;
        resolv_ = -1;
        if(ch_) ch_.resume();
    }

    void timer_base::reschedule() {
        at_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval_);
    }

    void timer_base::trigger() {
        if(resolv_ > -1) {
            // 主线程
            ++resolv_;
            // 协程可能未处于定时器等待状态
            if(ch_) ch_.resume();
        }
    }

    static php::value after(php::parameters& params) {
        auto tb = std::make_shared<timer_base>(params[0], params[1], false);
        sched_->insert(tb);
        return nullptr;
        // php::object obj(php::class_entry<after>::entry());
    }

    static php::value tick(php::parameters& params) {
        auto tb = std::make_shared<timer_base>(params[0], params[1], true);
        sched_->insert(tb);
        return nullptr;
    }

    void timer::declare(php::extension_entry& ext) {
        ext
            .function<after>("flame\\time\\after", {
                {"interval", php::TYPE::INTEGER},
                {"callback", php::TYPE::CALLABLE},
            })
            .function<tick>("flame\\time\\tick", {
                {"interval", php::TYPE::INTEGER},
                {"callback", php::TYPE::CALLABLE},
            });
        
        php::class_entry<timer> class_timer("flame\\time\\timer");
        class_timer
            .method<&timer::__construct>("__construct", {
                {"interval", php::TYPE::INTEGER},
                {"callback_or_repeat", php::TYPE::UNDEFINED, false, true},
                {"repeat", php::TYPE::BOOLEAN, false, true},
            })
            .method<&timer::start>("start", {
                {"callback", php::TYPE::CALLABLE, false, true},
            })
            .method<&timer::close>("close");
        
        ext.add(std::move(class_timer));
    }
    timer::timer()
    : tb_(nullptr)
    , start_(false) {

    }
    php::value timer::__construct(php::parameters& params) {
        std::size_t interval = params[0];
        bool repeat = true;
        php::callable cb;
        if(params.size() > 1) {
            if(params[1].type_of(php::TYPE::CALLABLE)) cb = params[1];
            else repeat = params[1].to_boolean();
        } 
        if(params.size() > 2) repeat = params[1].to_boolean();
        tb_ = std::make_shared<timer_base>(interval, cb, repeat);
        return nullptr;
    }

    php::value timer::start(php::parameters& params) {
        if(params.size() > 0) {
            tb_->cb_ = params[1];
        }
        if(start_) { // 删除上一个已经启动的定时器
            auto old = tb_;
            tb_ = std::make_shared<timer_base>(old->interval_, old->cb_, old->repeat_);
            sched_->remove(old);
        }
        start_ = true;
        sched_->insert(tb_); // 交由 sched_ 进行管理（释放）
        return nullptr;
    }
    php::value timer::close(php::parameters& params) {
        if(start_) sched_->remove(tb_);
        start_ = false;
        tb_.reset();
        return nullptr;
    }

}
