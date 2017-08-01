#pragma once
#include "../tcp_server.h"

namespace flame {
namespace net {
namespace http {
	// server_socket 仅注册，不提供直接使用的入口
	class server_socket {
	protected:
		uv_tcp_t     socket_;
		
		friend class server;
		friend class flame::net::stream_server;
	};
	class server: public tcp_server {
	public:
		inline php::value __destruct(php::parameters& params) {
			return tcp_server::__destruct(params);
		}
		inline php::value run(php::parameters& params) {
			return tcp_server::run(params);
		}
		inline php::value close(php::parameters& params) {
			return tcp_server::close(params);
		}
		inline php::value bind(php::parameters& params) {
			return tcp_server::bind(params);
		}
		php::value handle(php::parameters& params);
		// property local_address ""
		// property local_port 0
	protected:
		virtual void accept(uv_stream_t* s);
		virtual uv_stream_t* create_stream();
	private:
		std::map<std::string, php::callable> map_;
	};
}
}
}
