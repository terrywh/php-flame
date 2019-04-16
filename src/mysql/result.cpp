#include "../coroutine.h"
#include "result.h"
#include "_connection_lock.h"

namespace flame::mysql {

    void result::declare(php::extension_entry &ext) {
        php::class_entry<result> class_result("flame\\mysql\\result");
        class_result
            .property({"stored_rows", 0})
            .method<&result::fetch_row>("fetch_row")
            .method<&result::fetch_all>("fetch_all");

        ext.add(std::move(class_result));
    }
    
    php::value result::fetch_row(php::parameters &params) {
        php::array row(nullptr);
        if (cl_ && rs_) {
            coroutine_handler ch{coroutine::current};
            row = cl_->fetch(cl_->acquire(ch), rs_, f_, n_, ch);
        }
        if (row.type_of(php::TYPE::NULLABLE)) {
            // 尽早的释放连接
            rs_.reset();
            cl_.reset();
        }
        return row;
    }

    php::value result::fetch_all(php::parameters &params) {
        if (cl_ && rs_) {
            coroutine_handler ch{coroutine::current};
            auto conn = cl_->acquire(ch);
            php::array data {4}, row;

            for(row = cl_->fetch(conn, rs_, f_, n_, ch); !row.type_of(php::TYPE::NULLABLE); row = cl_->fetch(conn, rs_, f_, n_, ch)) {
                data.set(data.size(), row);
            }
            // 尽早的释放连接
            rs_.reset();
            cl_.reset();
            return data;
        }
        else {
            return nullptr;
        }
    }
} // namespace flame::mysql
