#pragma once
#include "../url.h"

namespace flame::http
{
    class _connection_pool : public std::enable_shared_from_this<_connection_pool>
    {
    public:
        _connection_pool(uint32_t cph);
        ~_connection_pool();
        using time_t_ = std::chrono::time_point<std::chrono::steady_clock>;
        std::shared_ptr<tcp::socket> acquire(std::shared_ptr<url> url, time_t_ timeout, coroutine_handler &ch);
        std::shared_ptr<tcp::socket> create(std::shared_ptr<url> u, time_t_ timeout, coroutine_handler& ch);
        php::value execute(client_request* req, time_t_ ts, coroutine_handler &ch);
        void release(std::string type, std::shared_ptr<tcp::socket> s);
        void sweep();
    private:

        const std::uint32_t max_;

        boost::asio::ip::tcp::resolver resolver_;
        std::map<std::string, int32_t> ps_; // pool_size
        std::multimap<std::string, std::pair<std::shared_ptr<tcp::socket>, time_t_>> p_; // connection pool
        std::map<std::string, std::list<std::function<void(std::shared_ptr<tcp::socket>)>>> wl_; // await list
        boost::asio::steady_timer tm_;
    };

}
