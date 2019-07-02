#pragma once
#include "../../vendor.h"
#include "../../coroutine_mutex.h"
#include "../coroutine.h"

namespace flame::udp {
    class server: public php::class_base {
    public:
        static void declare(php::extension_entry &ext);
        server();
        php::value __construct(php::parameters& params);
        php::value run(php::parameters& params);
        php::value send_to(php::parameters& params);
        php::value close(php::parameters& params);
    private:
        boost::asio::ip::udp::socket socket_;
        boost::asio::ip::udp::endpoint addr_;
        int concurrent_;
        int max_;
        bool closed_;
    };
}
