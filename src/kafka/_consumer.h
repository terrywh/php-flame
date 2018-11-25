#pragma once
#include "../vendor.h"
#include "../coroutine.h"

namespace flame::kafka
{
    class _consumer
    {
    public:
        _consumer(php::array& config, php::array& topics);
        ~_consumer();
        void subscribe(coroutine_handler& ch);
        php::object consume(coroutine_handler& ch);
        void commit(const php::object& msg, coroutine_handler& ch);
        void close(coroutine_handler& ch);
    private:
        rd_kafka_t* conn_;
        rd_kafka_topic_partition_list_t* tops_;
        // 此 strand 用于限制消费动作, 防止其堵塞所有工作线程
        boost::asio::io_context::strand  strd_;
    };
} // namespace flame::kafka
