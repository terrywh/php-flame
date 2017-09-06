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
		virtual int accept(uv_stream_t* server);
		void on_request(php::object& req, php::object& res);
	private:
		union {
			uv_stream_t server_;
			uv_pipe_t   server_pipe_;
			uv_tcp_t    server_tcp_;
		};
		bool unix_socket_;
		static void from_master_cb(uv_async_t* async);
		friend class server_connection;
	};
}
}
}
