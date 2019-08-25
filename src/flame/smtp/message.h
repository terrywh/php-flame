#pragma once
#include "../../vendor.h"
#include "../coroutine.h"
#include "smtp.h"

namespace flame::smtp {
    
    class message: public php::class_base {
    public:
        struct part_t {
            std::string content_type;
            std::string data;
        };

        static void declare(php::extension_entry& ext);
        php::value __construct(php::parameters& params);
        message();
        ~message();
        php::value from(php::parameters& params);
        php::value to(php::parameters& params);
        php::value cc(php::parameters& params);
        php::value subject(php::parameters& params);
        php::value append(php::parameters& params);
        php::value to_string(php::parameters& params);
    private:
        void build_ex();
        CURL*       c_easy_; // owner
        curl_slist* c_rcpt_; // owner

        int status_;

        std::string boundary_;

        std::size_t c_size_;
        php::buffer c_data_;
        php::string c_mail_;
        static std::size_t read_cb(char* buffer, std::size_t size, std::size_t n, void *data);
        
        CURLcode    c_code_;
        coroutine_handler c_coro_;

        static std::string random(int size = 16);
        friend class client;
    };
}
