#pragma once
#include "stream_handler.h"

namespace flame {
namespace net {
	class unix_socket: public php::class_base {
	public:
		unix_socket();
		php::value connect(php::parameters& params);
		inline php::value read(php::parameters& params) {
			return handler_.read(params);
		}
		php::value write(php::parameters& params) {
			return handler_.write(params);
		}
		php::value close(php::parameters& params) {
			return handler_.close(params);
		}
		// property remote_address ""
	private:
		stream_handler<uv_pipe_t, unix_socket> handler_;
		static void connect_cb(uv_connect_t* req, int status);
		friend class unix_server;
	};
}
}
