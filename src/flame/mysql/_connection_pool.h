#pragma once
#include "../../vendor.h"
#include "../../url.h"
#include "../coroutine.h"

#include "_connection_base.h"

namespace flame::mysql {

    class _connection_pool : public _connection_base, public std::enable_shared_from_this<_connection_pool> {
    public:
        _connection_pool(url u);
        ~_connection_pool();
        std::shared_ptr<MYSQL> acquire(coroutine_handler &ch) override;
        void sweep();
        void close();
    private:
        url                               url_;
        const std::uint16_t               min_;
        const std::uint16_t               max_;
        std::uint16_t                    size_;

        boost::asio::io_context::strand guard_; // 防止对下面队列操作发生多线程问题;
        std::list<std::function<void(std::shared_ptr<MYSQL>)>> await_;
        struct connection_t {
            MYSQL *conn;
            std::chrono::time_point<std::chrono::steady_clock> ttl;
        };
        std::list<connection_t> conn_;
        // 扫描闲置连接的定时器 (工作线程)
        boost::asio::steady_timer sweep_;

        int flag_;
        enum {
            FLAG_UNKNOWN        = 0x00,
            FLAG_REUSE_MASK     = 0x0f,
            FLAG_REUSE_BY_RESET = 0x01,
            FLAG_REUSE_BY_CUSER = 0x02,
            FLAG_REUSE_BY_PROXY = 0x04,
            FLAG_CHARSET_MASK   = 0xf0,
            FLAG_CHARSET_EQUAL  = 0x10,
            FLAG_CHARSET_DIFFER = 0x20,
        };

        void init_options(MYSQL *c);
        void release(MYSQL *c);
        void set_names(MYSQL* c);
        void query_charset(MYSQL* c);
        void query_version(MYSQL* c);
    };
} // namespace flame::mysql
