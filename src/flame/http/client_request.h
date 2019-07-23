#pragma once
#include "../../vendor.h"
#include "../../url.h"
#include "../coroutine.h"
#include "http.h"

namespace flame::http {
    
    class client_request: public php::class_base {
    public:
        static void declare(php::extension_entry& ext);
        php::value __construct(php::parameters& params);
        ~client_request();
        php::value http_version(php::parameters& params);
        php::value ssl_pem(php::parameters& params);
        php::value ssl_verify(php::parameters& params);
    private:
        void build_ex();
        CURL*       c_easy_ = nullptr; // cleanup in client
        curl_slist* c_head_ = nullptr; // owner
        
        friend class client;
        friend class _connection_pool;
    };
}
