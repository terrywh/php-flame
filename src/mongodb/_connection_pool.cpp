#include "../controller.h"
#include "mongodb.h"
#include "_connection_pool.h"

namespace flame::mongodb {
    
    _connection_pool::_connection_pool(const std::string& url)
    : guard_(gcontroller->context_y) {
        std::unique_ptr<mongoc_uri_t, void (*)(mongoc_uri_t *)> uri(mongoc_uri_new(url.c_str()), mongoc_uri_destroy);
        const bson_t* options = mongoc_uri_get_options(uri.get());
        if (!bson_has_field(options, MONGOC_URI_READPREFERENCE)) {
			mongoc_read_prefs_t* pref = mongoc_read_prefs_new(MONGOC_READ_SECONDARY_PREFERRED); // secondaryPreferred
			mongoc_uri_set_read_prefs_t(uri.get(), pref);
			mongoc_read_prefs_destroy(pref);
		}
        mongoc_uri_set_option_as_int32(uri.get(), MONGOC_URI_CONNECTTIMEOUTMS, 5000);
        mongoc_uri_set_option_as_int32(uri.get(), MONGOC_URI_MAXPOOLSIZE, 6);

        p_ = mongoc_client_pool_new(uri.get());
    }

    _connection_pool::~_connection_pool() {
        mongoc_client_pool_destroy(p_);
    }

    std::shared_ptr<mongoc_client_t> _connection_pool::acquire(coroutine_handler &ch) {
        std::shared_ptr<mongoc_client_t> conn;
        auto self = shared_from_this();
        boost::asio::post(guard_, [this, self, &conn, &ch] () {
            await_.push_back([this, self, &conn, &ch] (mongoc_client_t* c) {
                // 对应释放(归还)连接过程, 须持有当前对象的引用 self (否则当前对象可能先于连接释放被销毁)
                conn.reset(c, [this, self] (mongoc_client_t* c) {
                    boost::asio::post(guard_, std::bind(&_connection_pool::release, self, c));
                });
                ch.resume();
            });
            mongoc_client_t* c = mongoc_client_pool_try_pop(p_);
            if (c) release(c);
        });
        ch.suspend();
        return conn;
    }

    void _connection_pool::release(mongoc_client_t* c) {
        if (await_.empty()) mongoc_client_pool_push(p_, c);
        else {
            auto cb = await_.front();
            await_.pop_front();
            cb(c);
        }
    }
}
