#pragma once

namespace flame {
namespace http {
    template <class Stream>
    class executor: public std::enable_shared_from_this<executor<Stream>>, public boost::asio::coroutine {
    public:
        typedef Stream stream_type;
        executor(const php::object& cli, const php::object& req);
        void execute(const boost::system::error_code& error = boost::system::error_code(), std::size_t n = 0);
        void timeout(const boost::system::error_code& error);
    private:
        php::object                 cli_ref;
        php::object                 req_ref;
        php::object                 res_ref;
        client*                     cli_;
        client_request*             req_;
        client_response*            res_;
        std::shared_ptr<stream_type>  s_;
        boost::beast::flat_buffer cache_;
        std::shared_ptr<flame::coroutine> co_;
        boost::asio::steady_timer timer_;
    };
}
}
#include "executor.ipp"
