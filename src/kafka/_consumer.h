#pragma once
#include "../vendor.h"
#include "../coroutine.h"

namespace flame {
    template <class T>
    class coroutine_queue;
}

namespace flame::kafka {

    class _consumer {
    public:
        _consumer(php::array& config, php::array& topics);
        ~_consumer();
        void subscribe(coroutine_handler& ch);
        void consume(coroutine_queue<php::object>& q, coroutine_handler& ch);
        void commit(const php::object& msg, coroutine_handler& ch);
        void close(coroutine_handler& ch);
    private:
        rd_kafka_t* conn_;
        rd_kafka_topic_partition_list_t* tops_;
        bool       close_;
    };
} // namespace flame::kafka
