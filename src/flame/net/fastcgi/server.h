#pragma once
#include "../stream_server.h"

namespace flame {
namespace net {
namespace fastcgi {
	class server_connection;
	class server: public stream_server {
	public:
		server();
		php::value bind(php::parameters& params);
		php::value handle(php::parameters& params);
		php::value run(php::parameters& params);
		inline php::value close(php::parameters& params) {
			return stream_server::close(params);
		}
		// property local_address ""
	protected:
		php::callable                        handle_def_;
		std::map<std::string, php::callable> handle_map_;
		virtual void accept(uv_stream_t* s);
		virtual uv_stream_t* create_stream();
		void on_request(server_connection* conn, php::object&& req);
	private:
		uv_pipe_t server_;
		friend class server_connection;
	};
}
}
}
