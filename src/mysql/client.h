#pragma once
#include "../vendor.h"

namespace flame::mysql
{
    class _connection_pool;
    class client : public php::class_base
    {
    public:
        static void declare(php::extension_entry &ext);
        php::value __construct(php::parameters &params);
        php::value escape(php::parameters &params);
        php::value begin_tx(php::parameters &params);
        // php::value where(php::parameters &params);
        // php::value order(php::parameters &params);
        // php::value limit(php::parameters &params);
        php::value query(php::parameters &params);
        php::value insert(php::parameters &params);
        php::value delete_(php::parameters &params);
        php::value update(php::parameters &params);
        php::value select(php::parameters &params);
        php::value one(php::parameters &params);
        php::value get(php::parameters &params);

    protected:
        std::shared_ptr<_connection_pool> cp_;
        friend php::value connect(php::parameters &params);
    };
} // namespace flame::mysql
