#pragma once

namespace net { 
    class tcp_socket;
    namespace http {
        class request: public php::class_base {
            public:
                static void init(php::extension_entry& extension);
                static php::value parse(php::parameters& params);
                php::value __construct(php::parameters& params);
                void parse_request_head(request* r, php::object req, size_t n, php::callable done);
                void parse_request_body(request* r, php::object& req);

            private:
                php::buffer head;
                php::buffer body;
                php::array  hdr;
                boost::asio::streambuf buffer_;
                net::tcp_socket* tcp_;
                php::value done;

                friend class response;
        };
} }
