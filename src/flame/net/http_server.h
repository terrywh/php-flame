#pragma once 
#include "../promise.h"

namespace flame {
namespace net {
	class http_server {
	public:
		php::value __construct(php::parameters& params);
		php::value listen_and_serve(php::parameters& params);
		php::value handle(php::parameters& params);
		php::value handle_default(php::parameters& params);
		// property local_address null
	private:
		uv_tcp_t                             server_;
		std::map<std::string, php::callable> handle_;
	};
}
}