#pragma once
#include "stream_socket.h"

namespace flame {
namespace net {
	class tcp_socket: public stream_socket {
	public:
		tcp_socket();
		php::value connect(php::parameters& params);
		inline php::value read(php::parameters& params) {
			return stream_socket::read(params);
		}
		inline php::value write(php::parameters& params) {
			return stream_socket::write(params);
		}
		inline php::value close(php::parameters& params) {
			return stream_socket::close(params);
		}
		// property local_address ""
		// property remote_address ""
	private:
		uv_tcp_t    socket_;
		static void connect_cb(uv_connect_t* req, int status);
		void init_prop();

		friend class tcp_server;
	};
}
}
