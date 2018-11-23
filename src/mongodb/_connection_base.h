#pragma once
#include "../vendor.h"
#include "../coroutine.h"

namespace flame::mongodb {
    class _connection_base
    {
    public:
        virtual std::shared_ptr<mongoc_client_t> acquire(coroutine_handler& ch) = 0;
        php::value exec(std::shared_ptr<mongoc_client_t> conn, php::array& cmd, bool write, coroutine_handler& ch);

        static void fake_deleter(bson_t *doc);
    };
}