#pragma once
#include "../vendor.h"
#include "../coroutine.h"

namespace flame::mysql
{

    class _connection_base
    {
    public:
        // 此函数仅允许 escape 主线程调用
        static void escape(std::shared_ptr<MYSQL> c, php::buffer &b, const php::value &v, char quote = '\''); // 方便使用
        virtual std::shared_ptr<MYSQL> acquire(coroutine_handler& ch) = 0;

      protected:
    };
} // namespace flame::mysql
