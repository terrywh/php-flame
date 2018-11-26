#pragma once
#include "../vendor.h"
#include "../coroutine.h"
#include "../url.h"
#include "../coroutine_queue.h"

namespace flame::rabbitmq
{
    class _client
    {
    public:
        _client(url u, coroutine_handler& ch);
        
        void connect(coroutine_handler &ch);
        void consume(const std::string& queue, coroutine_queue<php::object>& q, coroutine_handler& ch);
        void confirm(std::uint64_t tag, coroutine_handler& ch);
        void reject(std::uint64_t tag, int flags, coroutine_handler& ch);
        void close_consumer(coroutine_handler& ch);
        void publish(const std::string& ex, const std::string& rk, const AMQP::Envelope& env, coroutine_handler& ch);
        void publish(const std::string& ex, const std::string& rk, const char* msg, size_t len, coroutine_handler& ch);
    private:
        AMQP::LibBoostAsioHandler hd_;
        AMQP::TcpConnection       cc_;
        AMQP::TcpChannel          ch_;
        int                       pf_;
        int                       fl_;
        std::string              tag_;

        coroutine_handler consumer_ch_;
        bool          consumer_closed_;

        friend class consumer;
    };
}
