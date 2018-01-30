#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "bulk_result.h"

namespace flame {
namespace db {
namespace mongodb {
	php::object bulk_result::create_from(bool success, bson_t* reply) {
		php::object rv = php::object::create<bulk_result>();
		rv.native<bulk_result>()->success_ = success;
		bson_iter_t i;
		bool r = bson_iter_init(&i, reply);
		assert(r);
		while(bson_iter_next(&i)) {
			const char* key = bson_iter_key(&i);
			if(strncmp(key, "nInserted", 9) == 0) {
				rv.prop("inserted") = bson_iter_as_int64(&i);
			}else if(strncmp(key, "nMatched", 8) == 0) {
				rv.prop("matched") = bson_iter_as_int64(&i);
			}else if(strncmp(key, "nModified", 9) == 0) {
				rv.prop("modified") = bson_iter_as_int64(&i);
			}else if(strncmp(key, "nRemoved", 8) == 0) {
				rv.prop("removed") = bson_iter_as_int64(&i);
			}else if(strncmp(key, "nUpserted", 9) == 0) {
				rv.prop("upserted") = bson_iter_as_int64(&i);
			}
		}
		return std::move(rv);
	}
}
}
}
