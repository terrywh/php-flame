#include "../controller.h"
#include "_connection_lock.h"
#include "mongodb.h"

namespace flame::mongodb {
    _connection_lock::_connection_lock(std::shared_ptr<mongoc_client_t> c)
    : conn_(c)
    , guard_(gcontroller->context_y) {
    }

    _connection_lock::~_connection_lock() {
    }
    
    std::shared_ptr<mongoc_client_t> _connection_lock::acquire(coroutine_handler &ch) {
        return conn_;
    }

    php::array _connection_lock::fetch(std::shared_ptr<mongoc_cursor_t> cs, coroutine_handler &ch) {
        bool has = false;
        auto err = std::make_shared<bson_error_t>();
        const bson_t *doc;
        boost::asio::post(guard_, [this, &cs, &ch, &has, &err, &doc]() {
            if (!mongoc_cursor_next(cs.get(), &doc)) has = mongoc_cursor_error(cs.get(), err.get());
            ch.resume();
        });
        ch.suspend();
        // 发生了错误
        if (has) throw php::exception(zend_ce_exception
                    , (boost::format("Failed to fetch document: %s") % err->message).str()
                    , err->code);
        // 文档 doc 仅为 "引用" 流程
        return bson2array(const_cast<bson_t*>(doc));
    }
} // namespace flame::mongodb
