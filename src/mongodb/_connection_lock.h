#pragma once
#include "../vendor.h"
#include "mongodb.h"
#include "_connection_base.h"

namespace flame::mongodb {

    class _connection_lock : public _connection_base, public std::enable_shared_from_this<_connection_lock> {
    public:
        _connection_lock(std::shared_ptr<mongoc_client_t> c);
        ~_connection_lock();
        std::shared_ptr<mongoc_client_t> acquire(coroutine_handler &ch) override;
        php::array fetch(std::shared_ptr<mongoc_cursor_t> cs, coroutine_handler &ch);
    private:
        boost::asio::io_context::strand guard_; // 防止对 cursor 的并行访问
        std::shared_ptr<mongoc_client_t> conn_;
    };
} // namespace flame::mongodb
