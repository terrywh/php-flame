#pragma once
#include "../../vendor.h"
#include "../coroutine.h"
#include "http.h"

namespace flame::http {
    
    class client_response: public php::class_base {
    public:
        static void declare(php::extension_entry& ext);
        php::value to_string(php::parameters& params);
        // 声明为 ZEND_ACC_PRIVATE 禁止创建（不会被调用）
        php::value __construct(php::parameters& params);
    private:
        CURL*       c_easy_;
        php::array  c_head_;
        php::buffer c_body_;
        CURLcode c_final_ = CURLE_OK;
        char c_error_[CURL_ERROR_SIZE];
        coroutine_handler c_coro_;
        void build_ex();
        // 接收 curl 回调(响应数据)
        static size_t c_write_cb(char *ptr, size_t size, size_t nmemb, void *data);
        static size_t c_header_cb(char *buffer, size_t size, size_t nitems, void *data);
        friend class client;
    };
}
