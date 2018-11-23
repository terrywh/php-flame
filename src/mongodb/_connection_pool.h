#pragma once
#include "../vendor.h"
#include "_connection_base.h"

namespace flame::mongodb {

    class _connection_pool: public _connection_base, public std::enable_shared_from_this<_connection_pool>
    {
    public:
        _connection_pool(const std::string& url);
        ~_connection_pool();
        std::shared_ptr<mongoc_client_t> acquire(coroutine_handler &ch) override;
    private:
        mongoc_client_pool_t* p_;
    };
}
