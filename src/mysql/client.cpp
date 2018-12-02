#include "../coroutine.h"
#include "client.h"
#include "_connection_pool.h"
#include "_connection_lock.h"
#include "mysql.h"
#include "tx.h"

namespace flame::mysql
{
    void client::declare(php::extension_entry &ext)
    {
        php::class_entry<client> class_client("flame\\mysql\\client");
        class_client
            .constant({"AAAA",123})
            .method<&client::__construct>("__construct", {}, php::PRIVATE)
            .method<&client::__destruct>("__destruct")
            .method<&client::escape>("escape",
            {
                {"data", php::TYPE::UNDEFINED},
            })
            .method<&client::begin_tx>("begin_tx")
            .method<&client::query>("query", {
                {"sql", php::TYPE::STRING},
            })
            .method<&client::insert>("insert", {
                {"table", php::TYPE::STRING},
                {"rows", php::TYPE::ARRAY},
            })
            .method<&client::delete_>("delete", {
                {"table", php::TYPE::STRING},
                {"where", php::TYPE::UNDEFINED},
                {"order", php::TYPE::UNDEFINED, false, true},
                {"limit", php::TYPE::UNDEFINED, false, true},
            })
            .method<&client::update>("update", {
                {"table", php::TYPE::STRING},
                {"where", php::TYPE::UNDEFINED},
                {"set", php::TYPE::UNDEFINED},
                {"order", php::TYPE::UNDEFINED, false, true},
                {"limit", php::TYPE::UNDEFINED, false, true},
            })
            .method<&client::select>("select", {
                {"table", php::TYPE::STRING},
                {"fields", php::TYPE::UNDEFINED},
                {"where", php::TYPE::UNDEFINED},
                {"order", php::TYPE::UNDEFINED, false, true},
                {"limit", php::TYPE::UNDEFINED, false, true},
            })
            .method<&client::one>("one", {
                {"table", php::TYPE::STRING},
                {"where", php::TYPE::UNDEFINED},
                {"order", php::TYPE::UNDEFINED, false, true},
            })
            .method<&client::get>("get", {
                {"table", php::TYPE::STRING},
                {"field", php::TYPE::STRING},
                {"where", php::TYPE::UNDEFINED},
                {"order", php::TYPE::UNDEFINED, false, true},
            });
        ext.add(std::move(class_client));
    }
    php::value client::__construct(php::parameters& params) {
        return nullptr;
    }
    php::value client::__destruct(php::parameters &params)
    {
        // std::cout << "client destruct\n";
        return nullptr;
    }
    php::value client::escape(php::parameters &params)
    {
        coroutine_handler ch {coroutine::current};
        std::shared_ptr<MYSQL> conn = cp_->acquire(ch);
        
        php::buffer buffer;
        char quote = '\'';
        if (params.size() > 1 && params[1].typeof(php::TYPE::STRING))
        {
            php::string q = params[1];
            if (q.data()[0] == '`') quote = '`';
        }
        _connection_base::escape(conn, buffer, params[0], quote);
        return std::move(buffer);
    }
    php::value client::begin_tx(php::parameters &params)
    {
        coroutine_handler ch{coroutine::current};
        auto conn = cp_->acquire(ch);
        auto cl = std::make_shared<_connection_lock>(conn);
        cl->begin_tx(ch);
        // 构建事务对象
        php::object obj(php::class_entry<tx>::entry());
        tx *ptr = static_cast<tx *>(php::native(obj));
        ptr->cl_ = cl; // 继续持有当前连接
        return std::move(obj);
    }
    php::value client::query(php::parameters &params)
    {
        coroutine_handler ch{coroutine::current};
        return cp_->query(cp_->acquire(ch), params[0], ch);
    }
    php::value client::insert(php::parameters &params)
    {
        coroutine_handler ch{coroutine::current};
        auto conn = cp_->acquire(ch);
        php::buffer buf;
        build_insert(conn, buf, params);
        return cp_->query(conn, std::string(buf.data(), buf.size()), ch);
    }
    php::value client::delete_(php::parameters &params)
    {
        coroutine_handler ch{coroutine::current};
        auto conn = cp_->acquire(ch);
        php::buffer buf;
        build_delete(conn, buf, params);
        return cp_->query(conn, std::string(buf.data(), buf.size()), ch);
    }
    php::value client::update(php::parameters &params)
    {
        coroutine_handler ch{coroutine::current};
        auto conn = cp_->acquire(ch);
        php::buffer buf;
        build_update(conn, buf, params);
        return cp_->query(conn, std::string(buf.data(), buf.size()), ch);
    }
    php::value client::select(php::parameters &params)
    {
        coroutine_handler ch{coroutine::current};
        auto conn = cp_->acquire(ch);
        php::buffer buf;
        build_select(conn, buf, params);
        return cp_->query(conn, std::string(buf.data(), buf.size()), ch);
    }
    php::value client::one(php::parameters &params)
    {
        coroutine_handler ch{coroutine::current};
        auto conn = cp_->acquire(ch);
        php::buffer buf;
        build_one(conn, buf, params);
        php::object rst = cp_->query(conn, std::string(buf.data(), buf.size()), ch);
        return rst.call("fetch_row");
    }
    php::value client::get(php::parameters &params)
    {
        coroutine_handler ch{coroutine::current};
        auto conn = cp_->acquire(ch);
        php::buffer buf;
        build_get(conn, buf, params);
        php::object rst = cp_->query(conn, std::string(buf.data(), buf.size()), ch);
        php::array row = rst.call("fetch_row");
        if(!row.empty()) {
            return row.get( static_cast<php::string>(params[1]) );
        }else{
            return nullptr;
        }
    }

} // namespace flame::mysql
