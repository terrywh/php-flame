#include "../coroutine.h"
#include "cursor.h"
#include "_connection_lock.h"

namespace flame::mongodb {

    void cursor::declare(php::extension_entry &ext) {
        php::class_entry<cursor> class_cursor("flame\\mongodb\\cursor");
        class_cursor
            .method<&cursor::__construct>("__construct", {}, php::PRIVATE)
            .method<&cursor::__destruct>("__destruct")
            .method<&cursor::fetch_row>("fetch_row")
            .method<&cursor::fetch_all>("fetch_all");
        ext.add(std::move(class_cursor));
    }

    php::value cursor::fetch_row(php::parameters &params) {
        php::array row(nullptr);
        if (cs_ && cl_) {
            coroutine_handler ch{coroutine::current};
            row = cl_->fetch(cs_, ch);
        }
        if (row.type_of(php::TYPE::NULLABLE)) {
            // 尽早释放连接
            cs_.reset();
            cl_.reset();
        }
        return row;
    }

    php::value cursor::fetch_all(php::parameters &params) {
        if (cs_ && cl_) {
            coroutine_handler ch{coroutine::current};
            php::array data(8);
            for(php::array row = cl_->fetch(cs_, ch); !row.type_of(php::TYPE::NULLABLE); row = cl_->fetch(cs_, ch))
                data.set(data.size(), row);
            // 尽早释放连接
            cs_.reset();
            cl_.reset();
            return data;
        }
        else return nullptr;
    }
} // namespace flame::mongodb
