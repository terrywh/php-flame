#pragma once
#include "coroutine.h"

namespace flame {
    template <class T>
    class coroutine_queue: private boost::noncopyable {
    public:
        coroutine_queue(std::size_t n = 1)
        : n_(n)
        , closed_(false) {
        }

        // !!!! 生产者进行关闭 !!!!
        void close() {
            closed_ = true;
            while (!c_.empty()) {
                auto ch = c_.extract(c_.begin()).value();
                ch.resume();
            }
        }
        bool is_closed() {
            return q_.empty() && closed_;
        }

        void push(const T& t, coroutine_handler& ch) {
            if (closed_) throw php::exception(zend_ce_error_exception
                , "Failed to push queue: already closed"
                , -1);
            if (q_.size() >= n_) {
                p_.insert(ch);
                ch.suspend();
            }
            q_.push_back(t);
            while(!q_.empty() && !c_.empty()) {
                auto ch = c_.extract(c_.begin()).value();
                ch.resume();
            }
        }
        std::optional<T> pop(coroutine_handler& ch) {
            for(;;) {
                if (!q_.empty()) break; // 有数据消费
                else if (closed_) return std::optional<T>();  // 无数据关闭
                else { // 无数据等待
                    c_.insert(ch);
                    ch.suspend();
                }
            }
            T t = q_.front();
            q_.pop_front();

            while (!p_.empty()) {
                auto ch = p_.extract(p_.begin()).value();
                ch.resume();
            }
            return std::optional<T>(t);
        }

    /* private: */
        std::size_t                 n_;
        std::list<T>                q_;
        std::set<coroutine_handler> c_; // 消费者
        std::set<coroutine_handler> p_; // 生产者
        bool                   closed_;
    };

    template <class T>
    std::shared_ptr < coroutine_queue<T> > select_queue(std::vector < std::shared_ptr<coroutine_queue<T>> > queues, coroutine_handler &ch) {
TRY_ALL:
        bool all_closed = true;
        for(auto i=queues.begin();i!=queues.end();++i) {
            if (!(*i)->q_.empty()) return *i;
            else if (!(*i)->closed_) all_closed = false;
        }
        if (all_closed) return std::shared_ptr< coroutine_queue<T> >(nullptr);
        for(auto i=queues.begin();i!=queues.end();++i) (*i)->c_.insert(ch);
        ch.suspend();
        for (auto i = queues.begin(); i != queues.end(); ++i) (*i)->c_.erase(ch);
        goto TRY_ALL;
    }

}
