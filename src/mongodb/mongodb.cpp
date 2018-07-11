#include "../controller.h"
#include "../coroutine.h"
#include "mongodb.h"
#include "client.h"
#include "_connection_base.h"
#include "_connection_pool.h"
#include "_connection_lock.h"
#include "cursor.h"
#include "object_id.h"
#include "date_time.h"
#include "collection.h"

namespace flame {
namespace mongodb {
	void declare(php::extension_entry& ext) {
		ext
			.on_module_startup([] (php::extension_entry& ext) -> bool {
				mongoc_init();
				return true;
			})
			.on_module_shutdown([] (php::extension_entry& ext) -> bool {
				mongoc_cleanup();
				return true;
			});
		ext
			.function<connect>("flame\\mongodb\\connect");
		client::declare(ext);
		cursor::declare(ext);
		object_id::declare(ext);
		date_time::declare(ext);
		collection::declare(ext);
	}
	php::value connect(php::parameters& params) {
		php::object cli(php::class_entry<client>::entry());
		client* cli_ = static_cast<client*>(php::native(cli));

		cli_->p_.reset(new _connection_pool(params[0]));
		std::shared_ptr<coroutine> co = coroutine::current;
		cli_->p_->exec([] (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_error_t> error) -> std::shared_ptr<bson_t> {
			return nullptr;
		}, [co, cli] (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_t> b, std::shared_ptr<bson_error_t> error) {
			co->resume(cli);
		});
		return coroutine::async();
	}
	php::value convert(bson_iter_t* i) {
		switch(bson_iter_type(i)) {
		case BSON_TYPE_DOUBLE:
			return bson_iter_double(i);
		case BSON_TYPE_UTF8: {
			std::uint32_t size = 0;
			const char* data = bson_iter_utf8(i, &size);
			return php::string(data, size);
		}
		case BSON_TYPE_DOCUMENT:
		case BSON_TYPE_ARRAY: {
			bson_iter_t j;
			bson_iter_recurse(i, &j);
			php::array  a(4);
			while(bson_iter_next(&j)) {
				a.set(bson_iter_key(&j), convert(&j));
			}
			return std::move(a);
		}
		case BSON_TYPE_BINARY: {
			std::uint32_t size = 0;
			const unsigned char* data;
			bson_iter_binary(i, nullptr, &size, &data);
			return php::string((const char*)data, size);
		}
		case BSON_TYPE_OID: {
			php::object o(php::class_entry<object_id>::entry());
			object_id* o_ = static_cast<object_id*>(php::native(o));
			bson_oid_copy(bson_iter_oid(i), &o_->oid_);
			return std::move(o);
		}
		case BSON_TYPE_BOOL:
			return bson_iter_bool(i);
		case BSON_TYPE_DATE_TIME: {
			php::object o(php::class_entry<date_time>::entry());
			date_time* o_ = static_cast<date_time*>(php::native(o));
			o_->tm_ = bson_iter_date_time(i);
			return o;
		}
		case BSON_TYPE_INT32:
			return bson_iter_int32(i);
		case BSON_TYPE_INT64:
			return bson_iter_int64(i);
		default:
			return nullptr;
		}
	}
	php::array convert(std::shared_ptr<bson_t> v) {
		php::array doc(4);

		bson_iter_t i;
		bson_oid_t  oid;
		bson_iter_init(&i, v.get());
		while(bson_iter_next(&i)) {
			doc.set(bson_iter_key(&i), convert(&i));
		}
		return std::move(doc);
	}
	void append_object(bson_t* doc, const php::string& key, const php::object& o) {
		if(o.instanceof(php_date_get_date_ce())) { // PHP 内置的 DateTime 类型
			bson_append_date_time(doc, key.c_str(), key.size(), static_cast<std::int64_t>(o.call("getTimestamp")) * 1000);
		}else if(o.instanceof(php::class_entry<date_time>::entry())) {
			date_time* o_ = static_cast<date_time*>(php::native(o));
			bson_append_date_time(doc, key.c_str(), key.size(), o_->tm_);
		}else if(o.instanceof(php::class_entry<object_id>::entry())) {
			object_id* o_ = static_cast<object_id*>(php::native(o));
			bson_append_oid(doc, key.c_str(), key.size(), &o_->oid_);
		}else{
			bson_append_null(doc, key.c_str(), key.size());
		}
	}
	std::shared_ptr<bson_t> convert(const php::array& v) {
		assert(v.typeof(php::TYPE::ARRAY));
		bson_t* doc = bson_new();
		for(auto i=v.begin(); i!=v.end(); ++i) {
			php::string key = i->first;
			key.to_string();
			php::value  val = i->second;
			switch(Z_TYPE_P(static_cast<zval*>(val))) {
			case IS_UNDEF:
				break;
			case IS_NULL:
				bson_append_null(doc, key.c_str(), key.size());
				break;
			case IS_TRUE:
				bson_append_bool(doc, key.c_str(), key.size(), true);
				break;
			case IS_FALSE:
				bson_append_bool(doc, key.c_str(), key.size(), false);
				break;
			case IS_LONG:
				bson_append_int64(doc, key.c_str(), key.size(), static_cast<std::int64_t>(val));
				break;
			case IS_DOUBLE:
				bson_append_double(doc, key.c_str(), key.size(), static_cast<double>(val));
				break;
			case IS_STRING: {
				php::string str = val;
				bson_append_utf8(doc, key.c_str(), key.size(), str.c_str(), str.size());
				break;
			}
			case IS_ARRAY: {
				auto a = convert(val);
				if(bson_has_field(a.get(), "0")) {
					bson_append_array(doc, key.c_str(), key.size(), a.get());
				}else{
					bson_append_document(doc, key.c_str(), key.size(), a.get());
				}
				break;
			}
			case IS_OBJECT:
				append_object(doc, key, val);
				break;
			}
		}
		return std::shared_ptr<bson_t>(doc, bson_destroy);
	}
	void null_deleter(bson_t* doc) {
		
	}
}
}