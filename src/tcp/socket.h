#pragma once
#include "../vendor.h"
#include "../coroutine.h"
#include "../coroutine_mutex.h"

namespace flame::tcp {

    class socket : public php::class_base {
    public:
        static void declare(php::extension_entry &ext);
        socket();
        php::value read(php::parameters &param);
        php::value write(php::parameters &params);
        php::value close(php::parameters &params);

    private:
        boost::asio::ip::tcp::socket socket_;
        boost::asio::streambuf       buffer_;
        coroutine_mutex              rmutex_;
        coroutine_mutex              wmutex_;

        friend class server;
        friend php::value connect(php::parameters &params);
    };
} // namespace flame::tcp
