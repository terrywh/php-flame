#pragma once
#include "../vendor.h"

namespace flame::tcp {
    class server : public php::class_base {
    public:
        static void declare(php::extension_entry &ext);
        server();
        php::value __construct(php::parameters &params);
        php::value run(php::parameters &params);
        php::value close(php::parameters &params);
    private:
        boost::asio::ip::tcp::acceptor acceptor_;
		boost::asio::ip::tcp::socket     socket_;
        boost::asio::ip::tcp::endpoint     addr_;
        php::callable                        cb_;
        bool                             closed_;
    };
} // namespace flame::tcp
