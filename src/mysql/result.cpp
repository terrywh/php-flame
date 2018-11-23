#include "../coroutine.h"
#include "result.h"
#include "_connection_lock.h"

namespace flame::mysql
{
    void result::declare(php::extension_entry &ext)
    {
        php::class_entry<result> class_result("flame\\mysql\\result");
        class_result
            .property({"stored_rows", 0})
            .method<&result::fetch_row>("fetch_row")
            .method<&result::fetch_all>("fetch_all");

        ext.add(std::move(class_result));
    }
    php::value result::fetch_row(php::parameters &params)
    {
        coroutine_handler ch{coroutine::current};
        return cl_->fetch(cl_->acquire(ch), rs_, f_, n_, ch);
    }
    php::value result::fetch_all(php::parameters &params)
    {
        coroutine_handler ch{coroutine::current};
        auto conn = cl_->acquire(ch);
        php::array data {4}, row;
        
        if(!rs_) return nullptr;
        for(row = cl_->fetch(conn, rs_, f_, n_, ch); !row.typeof(php::TYPE::NULLABLE); row = cl_->fetch(conn, rs_, f_, n_, ch)) {
            data.set(data.size(), row);
        }
        return data;
    }
} // namespace flame::mysql
