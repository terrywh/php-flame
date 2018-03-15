#pragma once

namespace flame {
namespace db {
namespace mongodb {
	class write_result: public php::class_base {
	public:
		php::value __construct(php::parameters& params) {
			return nullptr;
		}
		php::value has_errors(php::parameters& params);
		php::value has_write_errors(php::parameters& params);
		php::value has_write_concern_errors(php::parameters& params);
		static php::object create_from(bson_t* reply);
	};
}
}
}
