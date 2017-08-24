#pragma once
#include "stream_server.h"

namespace flame {
namespace net {
	class tcp_server: public stream_server {
	public:
		tcp_server();
		inline php::value run(php::parameters& params) {
			return stream_server::run_core(params);
		}
		inline php::value close(php::parameters& params) {
			return stream_server::close(params);
		}
		php::value handle(php::parameters& params);
		php::value bind(php::parameters& params);
		// property local_address ""
	protected:
		php::callable handle_;
		virtual int accept(uv_stream_t* server);
	private:
		uv_tcp_t      server_;
	};
}
}
