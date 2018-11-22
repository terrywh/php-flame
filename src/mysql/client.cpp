#include "../coroutine.h"
// #include "transaction.h"
#include "client.h"
#include "_connection_pool.h"
#include "mysql.h"
// #include "result.h"

namespace flame::mysql
{
    void client::declare(php::extension_entry &ext)
    {
        php::class_entry<client> class_client("flame\\mysql\\client");
        class_client
            .method<&client::__construct>("__construct", {}, php::PRIVATE)
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
        // std::shared_ptr<coroutine> co = coroutine::current;
        // c_->exec([](std::shared_ptr<MYSQL> c, int &error) -> MYSQL_RES*
        // { // 工作线程
        //     MYSQL *conn = c.get();
        //     error = mysql_real_query(conn, "START TRANSACTION", 17);
        //     assert(mysql_field_count(conn) == 0);
        //     return nullptr;
        // }, [co](std::shared_ptr<MYSQL> c, MYSQL_RES *r, int error) { // 主线程
        //     MYSQL *conn = c.get();
        //     if (error)
        //     {
        //         co->fail(mysql_error(conn), mysql_errno(conn));
        //     }
        //     else
        //     {
        //         php::object tx(php::class_entry<transaction>::entry());
        //         transaction *tx_ = static_cast<transaction *>(php::native(tx));
        //         tx_->c_.reset(new _connection_lock(c)); // 继续持有当前连接
        //         co->resume(std::move(tx));
        //     }
        // });
        // return coroutine::async();
        return nullptr;
    }
    php::value client::query(php::parameters &params)
    {
        // c_->query(coroutine::current, php::object(this), params[0]);
        // return coroutine::async();
        return nullptr;
    }
    php::value client::insert(php::parameters &params)
    {
        // php::buffer buf;
        // build_insert(c_, buf, params);
        // c_->query(coroutine::current, php::object(this), std::move(buf));
        // return coroutine::async();
        return nullptr;
    }
    php::value client::delete_(php::parameters &params)
    {
        // php::buffer buf;
        // build_delete(c_, buf, params);
        // c_->query(coroutine::current, php::object(this), std::move(buf));
        // return coroutine::async();
        return nullptr;
    }
    php::value client::update(php::parameters &params)
    {
        // php::buffer buf;
        // build_update(c_, buf, params);
        // c_->query(coroutine::current, php::object(this), std::move(buf));
        // return coroutine::async();
        return nullptr;
    }
    php::value client::select(php::parameters &params)
    {
        // php::buffer buf;
        // build_select(c_, buf, params);
        // c_->query(coroutine::current, php::object(this), std::move(buf));
        // return coroutine::async();
        return nullptr;
    }
    php::value client::one(php::parameters &params)
    {
        // php::buffer buf;
        // build_one(c_, buf, params);
        // result::stack_fetch(coroutine::current, php::object(this), php::string(nullptr));
        // c_->query(coroutine::current, php::object(this), std::move(buf));
        // return coroutine::async();
        return nullptr;
    }
    php::value client::get(php::parameters &params)
    {
        // php::buffer buf;
        // build_get(c_, buf, params);
        // result::stack_fetch(coroutine::current, php::object(this), params[1]);
        // c_->query(coroutine::current, php::object(this), std::move(buf));
        // return coroutine::async();
        return nullptr;
    }

} // namespace flame::mysql
