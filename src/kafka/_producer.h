#pragma once
#include "../vendor.h"
#include "../coroutine.h"

namespace flame::kafka {
    
    class _producer {
    public:
        _producer(php::array& config, php::array& topics);
        ~_producer();
        void publish(const php::string& topic, const php::string& key, const php::string& payload, const php::array& headers, coroutine_handler& ch);
        void flush(coroutine_handler& ch);
    private:
        rd_kafka_t* conn_;
        std::map<php::string, rd_kafka_topic_t*> tops_;
    };
} // namespace flame::kafka
