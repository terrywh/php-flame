#include "../coroutine.h"
#include "_connection_base.h"
#include "_connection_lock.h"
#include "cursor.h"
#include "mongodb.h"

namespace flame {
namespace mongodb {
	void _connection_base::execute(std::shared_ptr<coroutine> co, const php::object& ref, std::shared_ptr<bson_t> cmd, bool write) {
		// 调用子类 exec 存在捕获当前对象延长生命周期功能
		exec([this, cmd, write] (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_error_t> error) -> std::shared_ptr<bson_t> {
			mongoc_client_t* cli = c.get();
			const mongoc_uri_t *uri = mongoc_client_get_uri(cli);

			bson_t*     reply = bson_new();
			bson_error_t* err = new bson_error_t();
			int        result = 0;
			if(write) {
				result = mongoc_client_write_command_with_opts(cli, mongoc_uri_get_database(uri), cmd.get(), nullptr, reply, err);
			}else{
				result = mongoc_client_read_command_with_opts(cli, mongoc_uri_get_database(uri), cmd.get(), mongoc_uri_get_read_prefs_t(uri), nullptr, reply, err);
			}
			if(!result) error.reset(err);
			// 返回 reply 必须进行 bson_destroy 释放
			return std::shared_ptr<bson_t>(reply, bson_destroy);
		}, [co, ref] (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_t> b, std::shared_ptr<bson_error_t> error) {
			if(error) {
				co->fail(error->message, error->code);
				return;
			}
			if(bson_has_field(b.get(), "cursor")) {
				php::object o(php::class_entry<cursor>::entry());
				cursor* o_ = static_cast<cursor*>(php::native(o));
				o_->p_.reset(new _connection_lock(c)); // 继续持有当前客户端指针 (不释放)
				o_->c_.reset(mongoc_cursor_new_from_command_reply_with_opts(c.get(), b.get(), nullptr), mongoc_cursor_destroy);
				// 对象 cursor 实际窃取了 b 持有的 bson_t
				*std::get_deleter<void(*)(bson_t*)>(b) = null_deleter;
				co->resume(std::move(o));
			}else{
				co->resume(convert(b));
			}
		});
	}

}
}