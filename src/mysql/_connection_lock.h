#pragma once
#include "../vendor.h"
#include "_connection_base.h"

namespace flame::mysql
{
    class _connection_lock : public _connection_base, public std::enable_shared_from_this<_connection_lock>
    {
    public:
        _connection_lock(std::shared_ptr<MYSQL> c);
        ~_connection_lock();
        std::shared_ptr<MYSQL> acquire(coroutine_handler& ch) override;
      private:
        std::shared_ptr<MYSQL> conn_;
    };
} // namespace flame::mysql
