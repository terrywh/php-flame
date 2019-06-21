#pragma once
#include "../vendor.h"
#include "http.h"

namespace flame::http {
    
    class client_body: public php::class_base {
    public:
        static void declare(php::extension_entry& ext);
    };
}