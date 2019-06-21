#pragma once
#include "../vendor.h"
#include "mysql.h"

namespace flame::mysql {

    class _connection_lock;
    class tx : public php::class_base {
    public:
        static void declare(php::extension_entry &ext);
        php::value __construct(php::parameters &params); // 私有
        php::value __destruct(php::parameters& params);
        php::value commit(php::parameters &params);
        php::value rollback(php::parameters &params);

        php::value escape(php::parameters &params);
        php::value query(php::parameters &params);

        php::value insert(php::parameters &params);
        php::value delete_(php::parameters &params);
        php::value update(php::parameters &params);
        php::value select(php::parameters &params);
        php::value one(php::parameters &params);
        php::value get(php::parameters &params);

    protected:
        std::shared_ptr<_connection_lock> cl_;
        bool done_ = false;
        friend class client;
    };
} // namespace flame::mysql
