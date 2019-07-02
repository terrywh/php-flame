#pragma once
#include "../../vendor.h"
#include "mongodb.h"

namespace flame::mongodb {
    
    class date_time: public php::class_base {
    public:
        static void declare(php::extension_entry& ext);
        php::value __construct(php::parameters& params);
        php::value to_string(php::parameters& params);
        php::value unix(php::parameters& params);
        php::value unix_ms(php::parameters& params);
        php::value to_datetime(php::parameters& params);
        php::value to_json(php::parameters& params);
        php::value iso(php::parameters& params);
    private:
        std::int64_t tm_;
        friend php::value iter2value(bson_iter_t *i);
        friend std::shared_ptr<bson_t> array2bson(const php::array &v);
    };
}
