#pragma once
#include "../vendor.h"
#include <mongoc.h>

namespace flame::mongodb {
    void declare(php::extension_entry &ext);
    php::value connect(php::parameters &params);
    php::value iter2value(bson_iter_t *i);
    php::array bson2array(std::shared_ptr<bson_t> v);
    php::array bson2array(bson_t* v);
    std::shared_ptr<bson_t> array2bson(const php::array &v);
} // namespace flame::mongodb
