#include "../../fiber.h"
#include "mongodb.h"
#include "object_id.h"
#include "date_time.h"
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
		php::class_entry<object_id> class_object_id("flame\\db\\mongodb\\object_id");
		class_object_id.implements_json_serializable();
		class_object_id.add<&object_id::__construct>("__construct");
		class_object_id.add<&object_id::__toString>("__toString");
		class_object_id.add<&object_id::__toString>("toString");
		class_object_id.add<&object_id::jsonSerialize>("__debugInfo");
		class_object_id.add<&object_id::jsonSerialize>("jsonSerialize");
		class_object_id.add<&object_id::timestamp>("timestamp");
		ext.add(std::move(class_object_id));
		php::class_entry<date_time> class_date_time("flame\\db\\mongodb\\date_time");
		class_date_time.implements_json_serializable();
		class_date_time.add<&date_time::__construct>("__construct");
		class_date_time.add<&date_time::__toString>("__toString");
		class_date_time.add<&date_time::__toString>("toString");
		class_date_time.add<&date_time::jsonSerialize>("__debugInfo");
		class_date_time.add<&date_time::jsonSerialize>("jsonSerialize");
		class_date_time.add<&date_time::timestamp>("timestamp");
		ext.add(std::move(class_date_time));
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

		php::class_entry<collection> class_collection("flame\\db\\mongodb\\collection");
		class_collection.add<&collection::__debugInfo>("__debugInfo");
		class_collection.add<&collection::count>("count");
		ext.add(std::move(class_collection));
	}
}
}
}
