#pragma once
#include "../vendor.h"
#include "../coroutine.h"
#include "http.h"

namespace flame::http {
    class server_response;
    class server_request;

    template <bool isRequest>
    class value_body;
    class server;
    class _handler : public std::enable_shared_from_this<_handler> {
    public:
        _handler(server *svr, boost::asio::ip::tcp::socket &&sock);
        ~_handler();
        void start();
        
        void read_request();
        void call_handler();

        void write_head(server_response* res_ptr, coroutine_handler& ch);
        void write_body(server_response* res_ptr, php::string data, coroutine_handler& ch);
        void write_end(server_response* res_ptr, coroutine_handler& ch);
        void write_file(server_response* res_ptr, std::string file, coroutine_handler& ch);

        // 由 server_response 销毁时调用
        void finish(server_response *res, coroutine_handler &ch);

      private:
        server *svr_ptr;
        php::object svr_obj;
        boost::asio::ip::tcp::socket socket_;

        
        // std::shared_ptr<boost::beast::http::message<true, value_body<true>>> req_;
        std::shared_ptr<boost::beast::http::request_parser<value_body<true>>> req_;
        std::shared_ptr<boost::beast::http::message<false, value_body<false>>> res_;
        boost::beast::flat_buffer buffer_;
    };
    // enum
    // {
    //     RESPONSE_STATUS_HEADER_BUILT = 0x01,
    //     RESPONSE_STATUS_HEADER_SENT = 0x02,
    //     RESPONSE_STATUS_FINISHED = 0x04,
    //     RESPONSE_STATUS_DETACHED = 0x08,

    //     RESPONSE_TARGET_WRITE_HEADER = 0x01,
    //     RESPONSE_TARGET_WRITE_CHUNK = 0x02,
    //     RESPONSE_TARGET_WRITE_CHUNK_LAST = 0x03,
    // };
} // namespace flame::http
