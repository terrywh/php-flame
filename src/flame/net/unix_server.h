#pragma once
#include "stream_server.h"

namespace flame {
namespace net {
	class unix_server: public stream_server {
	public:
		unix_server();
		inline php::value run(php::parameters& params) {
			return stream_server::run(params);
		}
		inline php::value close(php::parameters& params) {
			return stream_server::close(params);
		}
		php::value handle(php::parameters& params);
		php::value bind(php::parameters& params);
		// property local_address ""
	protected:
		php::callable  handle_;
		virtual void accept(uv_stream_t* s);
		virtual uv_stream_t* create_stream();
	private:
		uv_pipe_t      server_;
	};
}
}