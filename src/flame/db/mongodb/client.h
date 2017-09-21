#pragma once

namespace flame {
namespace db {
namespace mongodb {
	class client: public php::class_base {
	public:
		php::value __construct(php::parameters& params);
		php::value __destruct(php::parameters& params);
		php::value collection(php::parameters& params);
		inline php::value __get(php::parameters& params) {
			return collection(params[0]);
		}
		inline php::value __isset(php::parameters& params) {
			return true;
		}
		php::value close(php::parameters& params);
		php::object collection(const php::string& name);
	private:
		mongoc_client_t* client_ = nullptr;
		mongoc_uri_t*    uri_ = nullptr;
	};
}
}
}
