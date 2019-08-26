#pragma once
#include "../../vendor.h"
#include "smtp.h"

namespace flame::http {
    class client_poll;
}
namespace flame::smtp {
    
    class client: public php::class_base {
    public:
        static void declare(php::extension_entry& ext);
        client();
        ~client();
        php::value __construct(php::parameters& params);
        php::value post(php::parameters& params);
    private:
        std::string c_rurl_; // 服务器地址 smtp://user:pass@xxx.xxx.xxx.xxx:port/
        std::string c_from_; // 解析后的 URL 用于设置 FROM 信息
        CURLM *c_multi_ = nullptr;
        int    c_still_ = 0;
        boost::asio::steady_timer c_timer_;

        php::value post_ex(const php::object& req);
        static int c_socket_cb(CURL* e, curl_socket_t fd, int action, void* cbp, void* data);
        static int c_timer_cb(CURLM *m, long timeout_ms, void* data);
        static curl_socket_t c_socket_open_cb(void* data, curlsocktype purpose, struct curl_sockaddr* address);
        static int c_socket_close_cb(void* data, curl_socket_t fd);
        static void c_socket_ready_cb(const boost::system::error_code& error, http::client_poll* poll, curl_socket_t fd, int action);
        void check_done();

        friend php::value connect(php::parameters& params);
    };
}
