#pragma once
#include "../vendor.h"
#include "mysql.h"

namespace flame::mysql {

    class _connection_lock;
    class result: public php::class_base {
    public:
        static void declare(php::extension_entry &ext);
        php::value fetch_row(php::parameters &params);
        php::value fetch_all(php::parameters &params);

    private:
        std::shared_ptr<_connection_lock> cl_;
        std::shared_ptr<MYSQL_RES>        rs_;
        MYSQL_FIELD                       *f_;
        unsigned int                       n_; // Number Of Fields
        friend class _connection_base;
    };
} // namespace flame::mysql
