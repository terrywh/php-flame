#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "mongodb.h"
#include "write_result.h"

namespace flame {
namespace db {
namespace mongodb {
	php::object write_result::create_from(bson_t* reply) {
		php::object rv = php::object::create<write_result>();
		bson_iter_t iter;
		bson_iter_init_find(&iter, reply, "writeErrors");
		rv.prop("write_errors") = from(&iter);
		bson_iter_init_find(&iter, reply, "writeConcernErrors");
		rv.prop("write_concern_errors") = from(&iter);
		bson_iter_init_find(&iter, reply, "insertedCount");
		rv.prop("inserted_count") = from(&iter);
		bson_iter_init_find(&iter, reply, "deletedCount");
		rv.prop("deleted_count") = from(&iter);
		bson_iter_init_find(&iter, reply, "modifiedCount");
		rv.prop("modified_count") = from(&iter);
		bson_iter_init_find(&iter, reply, "matchedCount");
		rv.prop("matched_count") = from(&iter);
		bson_iter_init_find(&iter, reply, "upsertedId");
		rv.prop("upserted_id") = from(&iter);
		
		return std::move(rv);
	}
	php::value write_result::has_errors(php::parameters& params) {
		return prop("write_errors").length() > 0 || prop("write_concern_errors").length() > 0;
	}
	php::value write_result::has_write_errors(php::parameters& params) {
		return prop("write_errors").length() > 0;
	}
	php::value write_result::has_write_concern_errors(php::parameters& params) {
		return prop("write_concern_errors").length() > 0;
	}
}
}
}
