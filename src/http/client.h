#pragma once
#include "../vendor.h"

namespace flame::http {
    class client_poll;
    class client: public php::class_base {
    public:
        static void declare(php::extension_entry& ext);
        client();
        ~client();
        php::value __construct(php::parameters& params);
        php::value exec(php::parameters& params);
        php::value get(php::parameters& params);
        php::value post(php::parameters& params);
        php::value put(php::parameters& params);
        php::value delete_(php::parameters& params);

        std::map<curl_socket_t, boost::asio::ip::tcp::socket*> c_socks_;

    private:
        CURLM *c_multi_ = nullptr;
        int    c_still_ = 0;
        boost::asio::steady_timer c_timer_;
        ;

        php::value exec_ex(const php::object& req);
        static int c_socket_cb(CURL* e, curl_socket_t fd, int action, void* cbp, void* data);
        static int c_timer_cb(CURLM *m, long timeout_ms, void* data);
        static curl_socket_t c_socket_open_cb(void* data, curlsocktype purpose, struct curl_sockaddr* address);
        static int c_socket_close_cb(void* data, curl_socket_t fd);
        static void c_socket_ready_cb(const boost::system::error_code& error, client_poll* poll, curl_socket_t fd, int action);
        void check_done();
    };
}
