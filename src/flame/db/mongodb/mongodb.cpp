#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "mongodb.h"
#include "object_id.h"
#include "date_time.h"
#include "write_result.h"
#include "client.h"
#include "collection.h"
#include "cursor.h"

namespace flame {
namespace db {
namespace mongodb {

	void init(php::extension_entry& ext) {
		std::uintptr_t rpp(reinterpret_cast<std::uintptr_t>(mongoc_read_prefs_new(MONGOC_READ_PRIMARY))),
			rps(reinterpret_cast<std::uintptr_t>(mongoc_read_prefs_new(MONGOC_READ_SECONDARY))),
			rppp(reinterpret_cast<std::uintptr_t>(mongoc_read_prefs_new(MONGOC_READ_PRIMARY_PREFERRED))),
			rpsp(reinterpret_cast<std::uintptr_t>(mongoc_read_prefs_new(MONGOC_READ_SECONDARY_PREFERRED))),
			rpn(reinterpret_cast<std::uintptr_t>(mongoc_read_prefs_new(MONGOC_READ_NEAREST)));
		ext.define({"flame\\db\\mongodb\\READ_PREFS_PRIMARY", php::value(rpp)});
		ext.define({"flame\\db\\mongodb\\READ_PREFS_SECONDARY", php::value(rps)});
		ext.define({"flame\\db\\mongodb\\READ_PREFS_PRIMARY_PREFERRED", php::value(rppp)});
		ext.define({"flame\\db\\mongodb\\READ_PREFS_SECONDARY_PREFERRED", php::value(rpsp)});
		ext.define({"flame\\db\\mongodb\\READ_PREFS_NEAREST", php::value(rpn)});
		
		ext.on_module_startup([] (php::extension_entry& ext) -> bool {
			mongoc_init();
			mongoc_log_trace_disable();
			mongoc_log_set_handler(nullptr, nullptr);
			return true;
		});
		ext.on_module_shutdown([] (php::extension_entry& ext) -> bool {
			mongoc_cleanup();
			return true;
		});
		// ---------------------------------------------------------------------
		php::class_entry<object_id> class_object_id("flame\\db\\mongodb\\object_id");
		class_object_id.implements_json_serializable();
		class_object_id.add<&object_id::__construct>("__construct");
		class_object_id.add<&object_id::to_string>("__toString");
		class_object_id.add<&object_id::jsonSerialize>("__debugInfo");
		class_object_id.add<&object_id::jsonSerialize>("jsonSerialize");
		class_object_id.add<&object_id::timestamp>("timestamp");
		ext.add(std::move(class_object_id));
		// ---------------------------------------------------------------------
		php::class_entry<date_time> class_date_time("flame\\db\\mongodb\\date_time");
		class_date_time.implements_json_serializable();
		class_date_time.add<&date_time::__construct>("__construct");
		class_date_time.add<&date_time::to_string>("__toString");
		class_date_time.add<&date_time::jsonSerialize>("__debugInfo");
		class_date_time.add<&date_time::jsonSerialize>("jsonSerialize");
		class_date_time.add<&date_time::timestamp>("timestamp");
		class_date_time.add<&date_time::timestamp_ms>("timestamp_ms");
		class_date_time.add<&date_time::to_datetime>("to_datetime");
		ext.add(std::move(class_date_time));
		// ---------------------------------------------------------------------
		php::class_entry<client> class_client("flame\\db\\mongodb\\client");
		class_client.add<&client::__construct>("__construct");
		class_client.add<&client::__destruct>("__destruct");
		class_client.add<&client::connect>("connect");
		class_client.add<&client::collection>("collection");
		ext.add(std::move(class_client));
		// ---------------------------------------------------------------------
		php::class_entry<write_result> class_write_result("flame\\db\\mongodb\\write_result");
		class_write_result.prop({"write_errors", nullptr});
		class_write_result.prop({"write_concern_errors", nullptr});
		class_write_result.prop({"inserted_count", -1});
		class_write_result.prop({"deleted_count", -1});
		class_write_result.prop({"modified_count", -1});
		class_write_result.prop({"matched_count", -1});
		class_write_result.prop({"upserted_id", nullptr});
		class_write_result.add<&write_result::__construct>("__construct", ZEND_ACC_PRIVATE);
		class_write_result.add<&write_result::has_errors>("has_errors");
		class_write_result.add<&write_result::has_write_errors>("has_write_errors");
		class_write_result.add<&write_result::has_write_concern_errors>("has_write_concern_errors");
		ext.add(std::move(class_write_result));
		// ---------------------------------------------------------------------
		php::class_entry<collection> class_collection("flame\\db\\mongodb\\collection");
		class_collection.add<&collection::__construct>("__construct", ZEND_ACC_PRIVATE);
		class_collection.add<&collection::__destruct>("__destruct");
		class_collection.add<&collection::count>("count");
		class_collection.add<&collection::insert_one>("insert_one");
		class_collection.add<&collection::insert_many>("insert_many");
		class_collection.add<&collection::remove_one>("remove_one");
		class_collection.add<&collection::remove_many>("remove_many");
		class_collection.add<&collection::update_one>("update_one");
		class_collection.add<&collection::update_many>("update_many");
		class_collection.add<&collection::find_one>("find_one");
		class_collection.add<&collection::find_many>("find_many");
		class_collection.add<&collection::aggregate>("aggregate");
		ext.add(std::move(class_collection));

		php::class_entry<cursor> class_cursor("flame\\db\\mongodb\\cursor");
		class_cursor.add<&cursor::__construct>("__construct", ZEND_ACC_PRIVATE);
		class_cursor.add<&cursor::__destruct>("__destruct");
		class_cursor.add<&cursor::to_array>("to_array");
		class_cursor.add<&cursor::next>("next");
		ext.add(std::move(class_cursor));
	}

	void fill_with(bson_t* doc, const php::array& arr) {
		if(!arr.is_array()) return;
		php::array& a = const_cast<php::array&>(arr);
		for(auto i=a.begin(); i!=a.end(); ++i) {
			php::string key = i->first.to_string();
			switch(i->second.type()) {
			case IS_UNDEF:
			case IS_NULL:
			bson_append_null(doc, key.c_str(), key.length());
			break;

			case IS_FALSE:
			bson_append_bool(doc, key.c_str(), key.length(), false);
			break;

			case IS_TRUE:
			bson_append_bool(doc, key.c_str(), key.length(), true);
			break;

			case IS_LONG:
			case IS_DOUBLE:
			bson_append_int64(doc, key.c_str(), key.length(), i->second);
			break;

			case IS_STRING: 
			bson_append_utf8(doc, key.c_str(), key.length(), 
				static_cast<php::string&>(i->second).c_str(),
				static_cast<php::string&>(i->second).length());
			break;
			
			case IS_OBJECT:
			if(static_cast<php::object&>(i->second).is_instance_of<object_id>()) {
				object_id* oid = static_cast<php::object&>(i->second).native<object_id>();
				bson_append_oid(doc, key.c_str(), key.length(), &oid->oid_);
			}else if(static_cast<php::object&>(i->second).is_instance_of<date_time>()) {
				date_time* dt = static_cast<php::object&>(i->second).native<date_time>();
				bson_append_date_time(doc, key.c_str(), key.length(), dt->milliseconds_);
			}else{
				bson_append_utf8(doc, key.c_str(), key.length(), "<unknown object>", 16);
			}
			break;

			case IS_ARRAY:{
			php::array& item = static_cast<php::array&>(i->second);
			bson_t* child = bson_new();
			if(item.length() > 0) {
				if(item.has(0)) {
					bson_append_array_begin(doc, key.c_str(), key.length(), child);
				}else{
					bson_append_document_begin(doc, key.c_str(), key.length(), child);
				}
				fill_with(child, item);
				if(item.has(0)) {
					bson_append_array_end(doc, child);
				}else{
					bson_append_document_end(doc, child);
				}
			}else{
				bson_append_array_begin(doc, key.c_str(), key.length(), child);
				bson_append_array_end(doc, child);
			}
			break;
			}

			}
		}
	}
	void fill_with(php::array& arr, const bson_t* doc) {
		bson_iter_t i;
		if(!bson_iter_init(&i, doc)) return;
		while(bson_iter_next(&i)) {
			arr[bson_iter_key(&i)] = from(&i);
		}
	}
	php::value from(bson_iter_t* i) {
		switch(bson_iter_type(i)) {
		// 不支持这些个类型
		case BSON_TYPE_EOD:
		case BSON_TYPE_UNDEFINED:
		case BSON_TYPE_TIMESTAMP:
		case BSON_TYPE_DECIMAL128:
		case BSON_TYPE_REGEX:
		case BSON_TYPE_DBPOINTER:
		case BSON_TYPE_CODE:
		case BSON_TYPE_SYMBOL:
		case BSON_TYPE_CODEWSCOPE:
		case BSON_TYPE_MAXKEY:
		case BSON_TYPE_MINKEY:
			return php::value();
		case BSON_TYPE_NULL:
			return php::value(nullptr);
		case BSON_TYPE_BOOL:
			return bson_iter_bool(i);
		case BSON_TYPE_INT32:
			return bson_iter_int32(i);
		case BSON_TYPE_INT64:
			return bson_iter_int64(i);
		case BSON_TYPE_DOUBLE:
			return bson_iter_double(i);
		case BSON_TYPE_UTF8: {
			uint32_t    len;
			const char* str = bson_iter_utf8(i, &len);
			return php::string(str, len);
		}
		case BSON_TYPE_ARRAY: {
			bson_iter_t j;
			if(!bson_iter_recurse(i, &j)) return php::value();
			php::array  a(4);
			int         x = 0;
			while(bson_iter_next(&j)) {
				a[x++] = from(&j);
			}
			return std::move(a);
		}
		case BSON_TYPE_DOCUMENT: {
			bson_iter_t j;
			if(!bson_iter_recurse(i, &j)) return php::value();
			php::array  a(4);
			while(bson_iter_next(&j)) {
				a[bson_iter_key(&j)] = from(&j);
			}
			return std::move(a);
		}
		case BSON_TYPE_OID: {
			php::object obj = php::object::create<object_id>();
			object_id*  cpp = obj.native<object_id>();
			bson_oid_copy(bson_iter_oid(i), &cpp->oid_);
			return std::move(obj);
		}
		case BSON_TYPE_DATE_TIME: {
			php::object obj = php::object::create<date_time>();
			date_time*  cpp = obj.native<date_time>();
			cpp->milliseconds_ = bson_iter_date_time(i);
			return std::move(obj);
		}
		default:
			assert(0);
		}
	}
	stack_bson_t::stack_bson_t() {
		bson_init(&doc_);
	}
	stack_bson_t::stack_bson_t(const php::array& arr) {
		bson_init(&doc_);
		mongodb::fill_with(&doc_, arr);
	}
	void stack_bson_t::fill_with(const php::array& arr) {
		bson_destroy(&doc_); // 防止意外
		mongodb::fill_with(&doc_, arr);
	}
	stack_bson_t::~stack_bson_t() {
		bson_destroy(&doc_);
	}
	stack_bson_t::operator bson_t*() const {
		return (bson_t*)&doc_;
	}
	
}
}
}
