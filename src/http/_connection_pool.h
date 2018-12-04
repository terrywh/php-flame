#pragma once
#include "../url.h"

namespace flame::http
{
    class _connection_pool : public std::enable_shared_from_this<_connection_pool>
    {
        struct context_t;
    public:
        _connection_pool(uint32_t cph);
        ~_connection_pool();
        void acquire(std::shared_ptr<url> url, std::shared_ptr<context_t> ctx);
        void create(std::shared_ptr<url> u, std::shared_ptr<context_t> ctx);
        php::value execute(client_request* req, int32_t timeout, coroutine_handler &ch);
        void release(std::string type, std::shared_ptr<tcp::socket> s);
        void sweep();
    private:

        using time_t_ = std::chrono::time_point<std::chrono::steady_clock>;
        using queue_t = std::list<std::shared_ptr<context_t>>;

        struct context_t : public std::enable_shared_from_this<context_t>{
            context_t(coroutine_handler &ch, queue_t& q, int32_t to);
            void expire();
            void wait();
            bool timeout();
            void operator()(std::shared_ptr<tcp::socket>);

            // enum status  normal canceled
            enum {
                NORMAL  = 0x0,
                TIMEOUT = 0x1,
                WAIT    = 0x1 << 1,
                CONNECT = 0x1 << 2,
                DONE    = 0x1 << 3
            };
            std::shared_ptr<tcp::socket> c_;
            boost::asio::steady_timer    tm_;
            boost::system::error_code    ec_;
            coroutine_handler            ch_;
            queue_t                      &q_;
            int32_t                      to_;
            time_t_                      tp_;
        };

        const std::uint32_t max_;

        boost::asio::ip::tcp::resolver resolver_;
        std::map<std::string, int32_t> ps_; // pool_size
        std::multimap<std::string, std::pair<std::shared_ptr<tcp::socket>, time_t_>> p_; // connection pool
        std::map<std::string, queue_t> wl_; // await list
        boost::asio::steady_timer tm_;
    };

}
