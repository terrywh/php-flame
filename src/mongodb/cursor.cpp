#include "../coroutine.h"
#include "cursor.h"
#include "_connection_lock.h"

namespace flame::mongodb
{
    void cursor::declare(php::extension_entry &ext)
    {
        php::class_entry<cursor> class_cursor("flame\\mongodb\\cursor");
        class_cursor
            .method<&cursor::__construct>("__construct", {}, php::PRIVATE)
            .method<&cursor::fetch_row>("fetch_row")
            .method<&cursor::fetch_all>("fetch_all");
        ext.add(std::move(class_cursor));
    }
    php::value cursor::fetch_row(php::parameters &params)
    {
        coroutine_handler ch{coroutine::current};
        return cl_->fetch(cs_, ch);
    }
    php::value cursor::fetch_all(php::parameters &params)
    {
        coroutine_handler ch{coroutine::current};
        php::array data(8);
        for(php::array row = cl_->fetch(cs_, ch); !row.typeof(php::TYPE::NULLABLE); row = cl_->fetch(cs_, ch))
        {
            data.set(data.size(), row);
        }
        return data;
    }
} // namespace flame::mongodb
