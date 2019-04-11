#pragma once
#include "../vendor.h"

namespace flame::mongodb {

    class _connection_pool;
    class client : public php::class_base {
    public:
        static void declare(php::extension_entry &ext);
        php::value __construct(php::parameters &params);
        php::value dump(php::parameters &params);
        php::value execute(php::parameters &params);
        php::value __get(php::parameters &params);
        php::value __isset(php::parameters &params);
    private:
        std::shared_ptr<_connection_pool> cp_;
        friend php::value connect(php::parameters &params);
    };
} // namespace flame::mongodb
