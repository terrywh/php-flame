#pragma once

namespace flame {
namespace http {
	class client;
	extern client* client_;

	void declare(php::extension_entry& ext);
	php::value get(php::parameters& params);
	php::value post(php::parameters& params);
	php::value put(php::parameters& params);
	php::value delete_(php::parameters& params);
	php::value exec(php::parameters& params);

	php::string ctype_encode(boost::string_view ctype, const php::value& v);
	php::value ctype_decode(boost::string_view ctype, const php::string& v);
}
}