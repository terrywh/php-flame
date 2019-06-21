#pragma once
#include "../vendor.h"
#include "../coroutine.h"
#include "mongodb.h"

namespace flame::mongodb {

    class _connection_base {
    public:
        enum command_type_t {
            COMMAND_RAW,
            COMMAND_WRITE,
            COMMAND_READ,
            COMMAND_READ_WRITE,
        };
        virtual std::shared_ptr<mongoc_client_t> acquire(coroutine_handler& ch) = 0;
        php::value exec(std::shared_ptr<mongoc_client_t> conn
            , php::array& cmd, int type, coroutine_handler& ch
            , std::shared_ptr<mongoc_client_session_t> session = nullptr);

        static void fake_deleter(bson_t *doc);
    };
}
