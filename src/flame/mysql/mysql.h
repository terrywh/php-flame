#pragma once
#include "../../vendor.h"
#include <mariadb/mysql.h>

namespace flame::mysql {

    void declare(php::extension_entry &ext);
    php::value connect(php::parameters& params);

    void build_where(std::shared_ptr<MYSQL> cm, php::buffer &buf, const php::value &data);
    void build_order(std::shared_ptr<MYSQL> cm, php::buffer &buf, const php::value &data);
    void build_limit(std::shared_ptr<MYSQL> cm, php::buffer &buf, const php::value &data);
    void build_insert(std::shared_ptr<MYSQL> cm, php::buffer &buf, php::parameters &params);
    void build_delete(std::shared_ptr<MYSQL> cm, php::buffer &buf, php::parameters &params);
    void build_update(std::shared_ptr<MYSQL> cm, php::buffer &buf, php::parameters &params);
    void build_select(std::shared_ptr<MYSQL> cm, php::buffer &buf, php::parameters &params);
    void build_one(std::shared_ptr<MYSQL> cm, php::buffer &buf, php::parameters &params);
    void build_get(std::shared_ptr<MYSQL> cm, php::buffer &buf, php::parameters &params);
}
