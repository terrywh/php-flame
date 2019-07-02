#pragma once
#include "../../vendor.h"

namespace flame::udp {
    extern std::unique_ptr<boost::asio::ip::udp::resolver> resolver_;
    void declare(php::extension_entry &ext);
    php::value bind_and_listen(php::parameters& params);
    php::value connect(php::parameters &params);
    std::pair<std::string, std::string> addr2pair(const std::string& addr);
} // namespace flame::tcp
