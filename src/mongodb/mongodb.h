#pragma once

namespace flame {
namespace mongodb {
	void declare(php::extension_entry& ext);
	php::value connect(php::parameters& params);
	php::array convert(std::shared_ptr<bson_t> v);
	std::shared_ptr<bson_t> convert(const php::array& v);
	void append_object(bson_t* doc, const php::string& key, const php::object& o);
	void null_deleter(bson_t* doc);
}
}
