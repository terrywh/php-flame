#pragma once
#include "../vendor.h"
#include "../coroutine.h"
#include "_connection_base.h"

namespace flame::mysql {
    class _connection_lock : public _connection_base, public std::enable_shared_from_this<_connection_lock> {
    public:
        _connection_lock(std::shared_ptr<MYSQL> c);
        ~_connection_lock();
        std::shared_ptr<MYSQL> acquire(coroutine_handler& ch) override;
        void begin_tx(coroutine_handler& ch);
        void commit(coroutine_handler& ch);
        void rollback(coroutine_handler& ch);
      private:
        std::shared_ptr<MYSQL> conn_;
    };
} // namespace flame::mysql
