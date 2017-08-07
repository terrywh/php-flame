#pragma once
#include "../stream_server.h"

namespace flame {
namespace net {
namespace fastcgi {
	class server: public stream_server {
	public:
		server();
		php::value bind(php::parameters& params);
		php::value handle(php::parameters& params);
		inline php::value run(php::parameters& params) {
			return stream_server::run(params);
		}
		inline php::value close(php::parameters& params) {
			return stream_server::close(params);
		}
		// property local_address ""
	protected:
		php::callable                        handle_def_;
		std::map<std::string, php::callable> handle_map_;
		virtual void accept(uv_stream_t* s);
		virtual uv_stream_t* create_stream();
	private:
		uv_pipe_t server_;
	};
}
}
}
