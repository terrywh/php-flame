#pragma once
#include "../vendor.h"

namespace flame::kafka
{
    void declare(php::extension_entry &ext);
    php::value consume(php::parameters& params);
    php::value produce(php::parameters& params);
    
    rd_kafka_conf_t*    array2conf(const php::array& data);
    rd_kafka_headers_t* array2hdrs(const php::array& data);
    php::array          hdrs2array(rd_kafka_headers_t* hdrs);
} // namespace flame::kafka
