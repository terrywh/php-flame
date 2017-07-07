#pragma once 
#include "../promise.h"

namespace flame {
namespace net {
	class tcp_socket: public php::class_base, public flame::promise {
	public:
		tcp_socket();
		php::value connect(php::parameters& params);
		php::value read(php::parameters& params);
		php::value read_len(php::parameters& params);
		php::value read_sep(php::parameters& params);
		php::value write(php::parameters& params);
		// property local_address  null
		// property remote_address null
	private:
		uv_tcp_t socket_;
		static void getaddrinfo_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res);
	};
}
}