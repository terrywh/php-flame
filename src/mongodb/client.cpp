#include "../coroutine.h"
#include "_connection_pool.h"
#include "client.h"
#include "collection.h"

namespace flame::mongodb
{
    void client::declare(php::extension_entry &ext)
    {
        php::class_entry<client> class_client("flame\\mongodb\\client");
        class_client
            .method<&client::__construct>("__construct", {}, php::PRIVATE)
            .method<&client::execute>("execute",
            {
                {"command", php::TYPE::ARRAY},
                {"write", php::TYPE::BOOLEAN, false, true},
            })
            .method<&client::__get>("collection",
            {
                {"name", php::TYPE::STRING}
            })
            .method<&client::__get>("__get",
            {
                {"name", php::TYPE::STRING}
            })
            .method<&client::__isset>("__isset", 
            {
                {"name", php::TYPE::STRING}
            });
        ext.add(std::move(class_client));
    }
    php::value client::execute(php::parameters &params)
    {
        bool write = false;
        if (params.size() > 1) write = params[1].to_boolean();
        coroutine_handler ch {coroutine::current};
        auto conn_ = cp_->acquire(ch);
        
        php::array cmd = params[0];
        return cp_->exec(conn_, cmd, write, ch);
    }
    php::value client::__get(php::parameters &params)
    {
        php::object obj(php::class_entry<collection>::entry());
        collection *ptr = static_cast<collection *>(php::native(obj));
        ptr->cp_ = cp_;
        ptr->name_ = params[0];
        obj.set("name", params[0]);
        return std::move(obj);
    }
    php::value client::__isset(php::parameters &params)
    {
        return true;
    }
} // namespace flame::mongodb