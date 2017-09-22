#pragma once

namespace flame {
namespace db {
namespace mongodb {
	void init(php::extension_entry& ext);
	void fill_bson_with(bson_t* doc, php::array& arr);
}
}
}
