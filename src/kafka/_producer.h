#pragma once
#include "../vendor.h"
#include "../coroutine.h"

namespace flame::kafka {
    // 生产者
    // 目前的实现方式 poll 定时调用机制需要 shared_from_this 保证生命周期
    class _producer: public std::enable_shared_from_this<_producer> {
    public:
        _producer(php::array& config, php::array& topics);
        ~_producer();
        void publish(const php::string& topic, const php::string& key, const php::string& payload, const php::array& headers, coroutine_handler& ch);
        void flush(coroutine_handler& ch);
        void close(coroutine_handler& ch);
        void start();
    private:
        rd_kafka_t* conn_;
        std::map<php::string, rd_kafka_topic_t*> tops_;
        boost::asio::steady_timer poll_;
        bool close_;
        void poll(int expire = 970);
        static void on_error(rd_kafka_t* conn, int error, const char* reason, void* data);
        friend class producer;
    };
} // namespace flame::kafka
