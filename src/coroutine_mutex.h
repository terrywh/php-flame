#pragma once
#include "coroutine.h"

namespace flame {
    class coroutine_mutex {
    public:
        coroutine_mutex()
        : mt_(false) {

        }

        void lock(coroutine_handler& ch) {  
            while(mt_) {
                cm_.insert(&ch);
                ch.suspend();
            }
            mt_ = true;
        }

        bool try_lock() {
            if (!mt_) {
                mt_ = true;
                return true;
            }
            return false;
        }

        void unlock() {
            while(!cm_.empty()) {
                auto ch = cm_.extract(cm_.begin()).value();
                ch->resume();
            }
            mt_ = false;
        }
    private:
        std::set<coroutine_handler*> cm_;
        bool mt_;
    };
    class coroutine_guard {
    public:
        coroutine_guard(coroutine_mutex& cm, coroutine_handler& ch)
        : cm_(cm) {
            cm_.lock(ch);
        }
        ~coroutine_guard() {
            cm_.unlock();
        }
    private:
        coroutine_mutex   cm_;
    };
}