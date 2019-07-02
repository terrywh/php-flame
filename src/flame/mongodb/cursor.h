#pragma once
#include "../../vendor.h"
#include "mongodb.h"

namespace flame::mongodb {
    class _connection_lock;
    class cursor: public php::class_base {
    public:
        static void declare(php::extension_entry& ext);
        php::value __construct(php::parameters& params) { // 私有
            return nullptr;
        }
        php::value __destruct(php::parameters& params) {
            return nullptr;
        }
        php::value fetch_row(php::parameters& params);
        php::value fetch_all(php::parameters& params);
    private:
        // 须先销毁 指针 后归还 连接
        std::shared_ptr<_connection_lock> cl_;
        std::shared_ptr<mongoc_client_session_t> ss_;
        std::shared_ptr<mongoc_cursor_t>  cs_;
        friend class _connection_base;
    };
} // namespace flame::mongodb
