#pragma once

namespace flame {
    class url {
    public:
        url(const php::string& str);
        std::string schema;
        std::string   host;
        std::uint16_t port;
        std::string   path;
        std::map<std::string, std::string> query;
        std::string   user;
        std::string   pass;

        std::string   str();
   
    };
}