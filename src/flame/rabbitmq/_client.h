#pragma once
#include "../../vendor.h"
#include "../../url.h"
#include "../../coroutine_queue.h"
#include "../coroutine.h"
#include "rabbitmq.h"

namespace flame::rabbitmq {

    class _client: public std::enable_shared_from_this<_client> {
    public:
        _client(url u, coroutine_handler& ch);
        
        void consume(const std::string& queue, coroutine_queue<php::object>& q, coroutine_handler& ch);
        void confirm(std::uint64_t tag, coroutine_handler& ch);
        void reject(std::uint64_t tag, int flags, coroutine_handler& ch);
        void consumer_close(coroutine_handler& ch);
        void consumer_close();
        void publish(const std::string& ex, const std::string& rk, const AMQP::Envelope& env, coroutine_handler& ch);
        void publish(const std::string& ex, const std::string& rk, const char* msg, size_t len, coroutine_handler& ch);
        // void producer_cb(AMQP::DeferredPublisher& defer, coroutine_handler& ch);
        void heartbeat();
        bool has_error();
        const std::string& error();
    private:
        AMQP::LibBoostAsioHandler chl_;
        AMQP::TcpConnection       con_;
        AMQP::TcpChannel          chn_;
        int                       pf_;
        int                       fl_;
        // 心跳定时器(在工作线程创建)
        boost::asio::steady_timer heartb_tm_;
        std::string       consumer_tg_;
        coroutine_handler consumer_ch_;
        bool              producer_cb_;
        coroutine_handler producer_ch_;
        std::string       error_;

        friend class consumer;
        friend php::value consume(php::parameters& params);
        friend php::value produce(php::parameters& params);
    };
}
