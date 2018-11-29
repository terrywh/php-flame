#pragma once
#include "../url.h"

namespace flame::http
{
    class _connection_pool : public std::enable_shared_from_this<_connection_pool>
    {
        public:
            _connection_pool(uint32_t cph);
            ~_connection_pool();
            std::shared_ptr<tcp::socket> acquire(std::shared_ptr<url> url, int32_t timeout, coroutine_handler &ch);
            std::shared_ptr<tcp::socket> create(std::shared_ptr<url> u, int timeout, coroutine_handler& ch);
            php::value execute(std::string type, std::shared_ptr<tcp::socket> c, 
                    boost::beast::http::message<true, value_body<true>> &ctr_, 
                    int32_t timeout, 
                    coroutine_handler &ch);
            void release(std::string type, std::shared_ptr<tcp::socket> s);
            void sweep();
        private:

            const std::uint32_t max_;

            boost::asio::ip::tcp::resolver resolver_;
            using time_t = std::chrono::time_point<std::chrono::steady_clock>;
            std::multimap<std::string, std::pair<std::shared_ptr<tcp::socket>, time_t>> cp_; // connection_pool
            std::map<std::string, int32_t> ps_; // pool_size
            std::map<std::string, std::list<std::function<void(std::shared_ptr<boost::asio::ip::tcp::socket>)>>> await_;
            boost::asio::steady_timer tm_;
    };

}
