#pragma once

namespace flame {
namespace db {
namespace mongodb {
	void init(php::extension_entry& ext);
	void fill_with(bson_t* doc, const php::array& arr);
	void fill_with(php::array& arr, const bson_t* doc);
	php::value from(bson_iter_t* iter);
}
}
}
