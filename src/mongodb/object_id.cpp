#include "object_id.h"

namespace flame {
namespace mongodb {
	void object_id::declare(php::extension_entry& ext) {
		php::class_entry<object_id> class_object_id("flame\\mongodb\\object_id");
		class_object_id
			.method<&object_id::__construct>("__construct")
			.method<&object_id::to_string>("__toString")
			.method<&object_id::to_json>("__toJSON")
			.method<&object_id::to_datetime>("__toDateTime")
			.method<&object_id::unix>("unix")
			.method<&object_id::to_json>("jsonSerialize")
			.method<&object_id::to_json>("__debugInfo");
		ext.add(std::move(class_object_id));
	}
	php::value object_id::__construct(php::parameters& params) {
		bson_oid_init(&oid_, nullptr);
		return nullptr;
	}
	php::value object_id::to_string(php::parameters& params) {
		php::string str(24);
		bson_oid_to_string(&oid_, str.data());
		return std::move(str);
	}
	php::value object_id::unix(php::parameters& params) {
		return bson_oid_get_time_t(&oid_);
	}
	php::value object_id::to_datetime(php::parameters& params) {
		return php::datetime(bson_oid_get_time_t(&oid_) * 1000);
	}
	php::value object_id::to_json(php::parameters& params) {
		php::array oid(1);
		php::string str(24);
		bson_oid_to_string(&oid_, str.data());
		oid.set("$oid", str);
		return std::move(oid);
	}
}
}