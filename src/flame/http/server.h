#pragma once
#include "../../vendor.h"
#include "http.h"

namespace flame::http {
    class acceptor;
    class server : public php::class_base {
    public:
        static void declare(php::extension_entry &ext);

        server();

        php::value __construct(php::parameters &params);
        php::value before(php::parameters &params);
        php::value after(php::parameters &params);
        php::value put(php::parameters &params);
        php::value delete_(php::parameters &params);
        php::value post(php::parameters &params);
        php::value patch(php::parameters &params);
        php::value get(php::parameters &params);
        php::value head(php::parameters &params);
        php::value options(php::parameters &params);
        php::value run(php::parameters &params);
        php::value close(php::parameters &params);

    private:
        boost::asio::ip::tcp::endpoint addr_;
        boost::asio::ip::tcp::acceptor accp_;
        boost::asio::ip::tcp::socket   sock_;
        std::map<std::string, php::callable> cb_;
        bool closed_;

        friend class _handler;
    };
} // namespace flame::http
