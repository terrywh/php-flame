#pragma once

namespace flame {
namespace db {
namespace mongodb {
	class bulk_result: public php::class_base {
	public:
		inline php::value success(php::parameters& params) {
			return success_;
		}
		static php::object create_from(bool success, bson_t* reply);
	private:
		bool success_;
	};
}
}
}
