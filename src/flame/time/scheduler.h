#pragma once
#include "../../vendor.h"
#include "../coroutine.h"

namespace flame::time {
    class scheduler;
    extern scheduler* sched_;
    class timer_base;
    /**
     * 使用堆存储各定时器时间，并按照指定时间进行唤醒回调
     */
    class scheduler {
    public:
        static void declare(php::extension_entry& ext);
        scheduler();
        void insert(std::shared_ptr<timer_base> tm);
        void remove(std::shared_ptr<timer_base> tm);
        void start();
        void close();

        static bool compare(const std::shared_ptr<timer_base>& tm1, const std::shared_ptr<timer_base>& tm2);
    private:
        boost::asio::io_context::strand strand_;
        boost::asio::steady_timer tm_;
        std::priority_queue<std::shared_ptr<timer_base>, std::vector<std::shared_ptr<timer_base>>, decltype(scheduler::compare)*> st_;
        coroutine_handler         ch_;
        bool                   close_;
        
        void run();
    };
}