#include "../../fiber.h"
#include "mongodb.h"
#include "object_id.h"
#include "date_time.h"
#include "bulk_result.h"
#include "client.h"
#include "collection.h"

namespace flame {
namespace db {
namespace mongodb {
	void init(php::extension_entry& ext) {
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
		class_object_id.add<&object_id::__toString>("__toString");
		class_object_id.add<&object_id::__toString>("toString");
		class_object_id.add<&object_id::jsonSerialize>("__debugInfo");
		class_object_id.add<&object_id::jsonSerialize>("jsonSerialize");
		class_object_id.add<&object_id::timestamp>("timestamp");
		ext.add(std::move(class_object_id));
		// ---------------------------------------------------------------------
		php::class_entry<date_time> class_date_time("flame\\db\\mongodb\\date_time");
		class_date_time.implements_json_serializable();
		class_date_time.add<&date_time::__construct>("__construct");
		class_date_time.add<&date_time::__toString>("__toString");
		class_date_time.add<&date_time::__toString>("toString");
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
		class_client.add<&client::__isset>("__isset", {
			php::of_string("name"),
		});
		class_client.add<&client::__get>("__get", {
			php::of_string("name"),
		});
		class_client.add<&client::collection>("collection");
		class_client.add<&client::close>("close");
		ext.add(std::move(class_client));
		// ---------------------------------------------------------------------
		php::class_entry<bulk_result> class_bulk_result("flame\\db\\mongodb\\bulk_result");
		class_bulk_result.add(php::property_entry("inserted", 0));
		class_bulk_result.add(php::property_entry("matched", 0));
		class_bulk_result.add(php::property_entry("modified", 0));
		class_bulk_result.add(php::property_entry("removed", 0));
		class_bulk_result.add(php::property_entry("upserted", 0));
		class_bulk_result.add<&bulk_result::success>("success");
		ext.add(std::move(class_bulk_result));
		// ---------------------------------------------------------------------
		php::class_entry<collection> class_collection("flame\\db\\mongodb\\collection");
		class_collection.add<&collection::__debugInfo>("__debugInfo");
		class_collection.add<&collection::count>("count");
		class_collection.add<&collection::insert_one>("insert_one");
		class_collection.add<&collection::insert_many>("insert_many");
		ext.add(std::move(class_collection));
	}

	void fill_bson_with(bson_t* doc, php::array& arr) {
		for(auto i=arr.begin(); i!=arr.end(); ++i) {
			php::string key = i->first.to_string();
			php::value& val = i->second;
			switch(val.type()) {
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
					bson_append_int64(doc, key.c_str(), key.length(), val);
					break;
				case IS_STRING: {
					php::string& str = val;
					bson_append_utf8(doc, key.c_str(), key.length(), str.c_str(), str.length());
					break;
				}
				case IS_OBJECT: {
					php::object& obj = val;
					if(obj.is_instance_of<object_id>()) {
						object_id* oid = obj.native<object_id>();
						bson_append_oid(doc, key.c_str(), key.length(), &oid->oid_);
					}else if(obj.is_instance_of<date_time>()) {
						date_time* dt = obj.native<date_time>();
						bson_append_date_time(doc, key.c_str(), key.length(), dt->milliseconds_);
					}else{
						bson_append_utf8(doc, key.c_str(), key.length(), "<unknown object>", 16);
					}
					break;
				}
				case IS_ARRAY: {
					php::array& arr = val;
					bson_t* child = bson_new();
					if(arr.length() > 0) {
						if(arr.is_a_list()) {
							bson_append_array_begin(doc, key.c_str(), key.length(), child);
						}else{
							bson_append_document_begin(doc, key.c_str(), key.length(), child);
						}
						fill_bson_with(child, arr);
						if(arr.is_a_list()) {
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
}
}
}
