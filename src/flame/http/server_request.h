#pragma once
#include "../../vendor.h"
#include "http.h"

namespace flame::http {
    
    class handler;
    class server_request: public php::class_base {
    public:
        static void declare(php::extension_entry& ext);
        server_request();
        ~server_request();
        php::value __construct(php::parameters& params);
    private:
        void build_ex(const boost::beast::http::message<true, value_body<true>>& ctr);

        friend class _handler;
    };
}
