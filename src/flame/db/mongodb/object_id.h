#pragma once

namespace flame {
namespace db {
namespace mongodb {
	class object_id: public php::class_base {
	public:
		php::value __construct(php::parameters& params);
		php::value __toString(php::parameters& params);
		php::value jsonSerialize(php::parameters& params);
		php::value timestamp(php::parameters& params);
	private:
		bson_oid_t oid_;
		friend void fill_bson_with(bson_t* doc, php::array& arr);
	};

}
}
}
