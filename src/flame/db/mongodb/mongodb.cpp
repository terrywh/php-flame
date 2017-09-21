#include "../../fiber.h"
#include "mongodb.h"
#include "client.h"
#include "collection.h"

namespace flame {
namespace db {
namespace mongodb {
	void init(php::extension_entry& ext) {
		ext.on_module_startup([] (php::extension_entry& ext) -> bool {
			mongoc_init();
			return true;
		});
		ext.on_module_shutdown([] (php::extension_entry& ext) -> bool {
			mongoc_cleanup();
			return true;
		});
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
		class_collection.add<&collection::count>("count");
		ext.add(std::move(class_collection));
	}
}
}
}
