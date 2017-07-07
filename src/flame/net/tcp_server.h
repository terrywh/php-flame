#pragma once 
#include "../promise.h"

namespace flame {
namespace net {
	class tcp_server {
	public:
		php::value __construct(php::parameters& params);
		php::value listen(php::parameters& params);
		php::value accept(php::parameters& params);
		php::value close(php::parameters& params);
		// property local_address null
	private:
		uv_tcp_t server_;
	};
}
}