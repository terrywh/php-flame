#pragma once
#include "vendor.h"

namespace flame {
    class url {
    public:
        url(const php::string& str, bool parse_query=true);
        std::string schema;
        std::string   host;
        std::uint16_t port;
        std::string   path;
        std::map<std::string, std::string> query;
        std::string   user;
        std::string   pass;

        std::string   str(bool update = false, bool with_query = true);
    private:
        std::string raw_;
    };
}