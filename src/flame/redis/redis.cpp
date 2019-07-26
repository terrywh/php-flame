#include "redis.h"
#include "_connection_pool.h"
#include "client.h"
#include "tx.h"

namespace flame::redis {
    
    void declare(php::extension_entry& ext) {
        ext.function<connect>("flame\\redis\\connect", {
            {"uri", php::TYPE::STRING}
        });
        client::declare(ext);
        tx::declare(ext);
    }

    php::value connect(php::parameters &params) {
        url u(params[0]);
        php::object obj {php::class_entry<client>::entry()};
        client *ptr = static_cast<client *>(php::native(obj));
        ptr->cp_.reset(new _connection_pool(u));
        ptr->cp_->sweep(); // 启动自动清理扫描
        gcontroller->on_stop([cp = ptr->cp_] () {
            cp->sweep_.cancel();
        });
        return std::move(obj);
    }
}
