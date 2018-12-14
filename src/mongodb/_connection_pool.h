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
        void release(mongoc_client_t* c);
    private:
        mongoc_client_pool_t* p_;
        boost::asio::io_context::strand guard_; // 防止下面队列并行访问
        std::list<std::function<void(mongoc_client_t *)>> await_;
    };
}
