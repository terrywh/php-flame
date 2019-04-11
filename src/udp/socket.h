#pragma once
#include "../vendor.h"
#include "../coroutine.h"
#include "../coroutine_mutex.h"

namespace flame::udp {
    
    class socket : public php::class_base {
    public:
        static void declare(php::extension_entry &ext);
        socket();
        php::value __construct(php::parameters &param);
        php::value recv_from(php::parameters &param);
        php::value recv(php::parameters& params);
        php::value send_to(php::parameters &params);
        php::value send(php::parameters& params);
        php::value close(php::parameters &params);

    private:
        boost::asio::ip::udp::socket socket_;
        php::buffer                  buffer_;
        coroutine_mutex              rmutex_;
        bool                      connected_;
        int                             max_;

        friend class server;
        friend php::value connect(php::parameters &params);
    };
} // namespace flame::udp
