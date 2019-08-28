#pragma once
#include "../../vendor.h"
#include "../coroutine.h"

namespace flame::time {
    class timer_base: public std::enable_shared_from_this<timer_base> {
    public:
        timer_base(std::size_t ms, php::callable cb, bool repeat = false);
        ~timer_base();
        void start();
        void close();
        bool closed() {
            return repeat_ == false && resolv_ < 0;
        }
        void reschedule();
        virtual void trigger();
        const std::chrono::steady_clock::time_point& expire_at() const {
            return at_;
        }
    protected:
        std::chrono::steady_clock::time_point at_;
        php::callable                         cb_;
        coroutine_handler                     ch_;
        std::size_t                     interval_;
        int                               resolv_;
        bool                              repeat_;

        friend class scheduler;
        friend class timer;
    };


    class timer: public php::class_base {
    public:
        static void declare(php::extension_entry& ext);
        php::value __construct(php::parameters& params);
        php::value start(php::parameters& params);
        php::value close(php::parameters& params);

        timer();
        std::shared_ptr<timer_base> tb_;
        bool      start_;
    };

}