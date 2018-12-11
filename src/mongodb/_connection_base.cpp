#include "../controller.h"
#include "_connection_base.h"
#include "_connection_lock.h"
#include "mongodb.h"
#include "cursor.h"

namespace flame::mongodb
{
    void _connection_base::fake_deleter(bson_t *doc) {}
    php::value _connection_base::exec(std::shared_ptr<mongoc_client_t> conn, php::array& pcmd, bool write, coroutine_handler &ch)
    {
        auto cmd = array2bson(pcmd);
        std::shared_ptr<bson_t> rep(bson_new(), bson_destroy);
        auto err = std::make_shared<bson_error_t>();
        int  rok = 0;
        boost::asio::post(gcontroller->context_y, [conn, write, &cmd, &rep, &err, &rok, &ch] () {
            const mongoc_uri_t *uri = mongoc_client_get_uri(conn.get());
            if(write)
            {
                rok = mongoc_client_write_command_with_opts(conn.get(), mongoc_uri_get_database(uri), cmd.get(), nullptr, rep.get(), err.get());
            }
            else
            {
                rok = mongoc_client_read_command_with_opts(conn.get(), mongoc_uri_get_database(uri), cmd.get(), mongoc_uri_get_read_prefs_t(uri), nullptr, rep.get(), err.get());
            }
            ch.resume();
        });
        ch.suspend();
        if(!rok)
        {
            throw php::exception(zend_ce_exception,
                    (boost::format("failed to execute MongoDB command: (%1%) %2%") % err->code % err->message).str(),
                    err->code);
        }
        else if(bson_has_field(rep.get(), "cursor"))
        {
            php::object obj{php::class_entry<cursor>::entry()};
            auto ptr = static_cast<cursor *>(php::native(obj));
            ptr->cl_.reset(new _connection_lock(conn));
            ptr->cs_.reset(
                mongoc_cursor_new_from_command_reply_with_opts(conn.get(), rep.get(), nullptr),
                mongoc_cursor_destroy);
            // cursor 实际会窃取 rep 对应的 bson_t 结构; 
            // 按文档说会被上面函数 "销毁"
            // 参考: http://mongoc.org/libmongoc/current/mongoc_cursor_new_from_command_reply_with_opts.html
            *std::get_deleter<void (*)(bson_t*)>(rep) = fake_deleter;
            return std::move(obj);
        }
        else
        {
            php::array r(4);

            bson_iter_t i;
            bson_oid_t oid;
            bson_iter_init(&i, rep.get());
            while (bson_iter_next(&i))
            {
                const char* key = bson_iter_key(&i);
                std::uint32_t len = bson_iter_key_len(&i);
                if(len > 0 && key[0] == '$') continue;
                r.set(php::string(key, len), iter2value(&i));
            }
            return std::move(r);
        }
    }
} // namespace flame::mongodb