#include "../controller.h"
#include "_connection_base.h"
#include "_connection_lock.h"
#include "mongodb.h"
#include "cursor.h"

namespace flame::mongodb
{
    void _connection_base::fake_deleter(bson_t *doc) {}
    php::value _connection_base::exec(std::shared_ptr<mongoc_client_t> conn, php::array& pcmd, int type, coroutine_handler &ch)
    {
        std::shared_ptr<bson_t> cmd = array2bson(pcmd);
        // 此处不能使用 bson_new 创建 bson_t 否则会导致内存泄漏
        //（实际对 reply 的使用流程会重新初始化 reply 导致其值被至于 stack 上，导致此处 heap 内存泄漏）
        bson_t reply, option = BSON_INITIALIZER;
        std::shared_ptr<bson_t> rep(&reply, bson_destroy), opt(&option, bson_destroy);
        std::shared_ptr<bson_error_t> err = std::make_shared<bson_error_t>();
        std::uint32_t server_id;
        int  rok = 0;

        boost::asio::post(gcontroller->context_y, [conn, type, &server_id, &cmd, &rep, &opt, &err, &rok, &ch] () {
            mongoc_server_description_t* server;
            switch(type) {
            case COMMAND_READ:
                server = mongoc_client_select_server(conn.get(), false, nullptr, nullptr);
            case COMMAND_RAW:
            case COMMAND_WRITE:
            case COMMAND_READ_WRITE:
            default:
                server = mongoc_client_select_server(conn.get(), true, nullptr, nullptr);
            }
            server_id = mongoc_server_description_id(server);
            mongoc_server_description_destroy(server);
            bson_append_int32(opt.get(), "serverId", 8, server_id);
            switch(type) {
            case COMMAND_READ:
                rok = mongoc_client_read_command_with_opts(conn.get(), mongoc_uri_get_database(mongoc_client_get_uri(conn.get())), cmd.get(), nullptr, opt.get(), rep.get(), err.get());
                break;
            case COMMAND_WRITE:
                rok = mongoc_client_write_command_with_opts(conn.get(), mongoc_uri_get_database(mongoc_client_get_uri(conn.get())), cmd.get(), opt.get(), rep.get(), err.get());
                break;
            case COMMAND_READ_WRITE:
                rok = mongoc_client_read_write_command_with_opts(conn.get(), mongoc_uri_get_database(mongoc_client_get_uri(conn.get())), cmd.get(), nullptr, opt.get(), rep.get(), err.get());
                break;
            case COMMAND_RAW:
            default:
                rok = mongoc_client_command_with_opts(conn.get(), mongoc_uri_get_database(mongoc_client_get_uri(conn.get())), cmd.get(), nullptr, opt.get(), rep.get(), err.get());
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
                mongoc_cursor_new_from_command_reply_with_opts(conn.get(), rep.get(), opt.get()),
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
