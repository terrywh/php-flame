#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "object_id.h"

namespace flame {
namespace db {
namespace mongodb {
	php::value object_id::__construct(php::parameters& params) {
		if(params.length() >= 1) {
			if(params[0].is_string()) {
				php::string& id = params[0];
				if(bson_oid_is_valid(id.c_str(), id.length())) {
					bson_oid_init_from_string(&oid_, id.c_str());
					return nullptr;
				}
			}
			throw php::exception("invalid hex string for object_id");
		}else{
			bson_oid_init(&oid_, nullptr);
			return nullptr;
		}
	}
	php::value object_id::to_string(php::parameters& params) {
		php::string oid(24);
		bson_oid_to_string(&oid_, oid.data());
		return std::move(oid);
	}
	php::value object_id::jsonSerialize(php::parameters& params) {
		php::array  oid(1);
		php::string str(24);
		bson_oid_to_string(&oid_, str.data());
		oid["$oid"] = std::move(str);
		return std::move(oid);
	}
	php::value object_id::timestamp(php::parameters& params) {
		return bson_oid_get_time_t(&oid_);
	}
}
}
}
