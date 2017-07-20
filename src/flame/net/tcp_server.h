#pragma once

namespace flame {
namespace net {
	class tcp_server: public php::class_base {
	public:
		tcp_server();
		php::value __destruct(php::parameters& params);
		php::value handle(php::parameters& params);
		php::value bind(php::parameters& params);
		php::value run(php::parameters& params);
		php::value close(php::parameters& params);
		// property local_address ""
		// property local_port 0
	private:
		uv_tcp_t      server_;
		php::callable handle_;
		static void connection_cb(uv_stream_t* server, int status);
		static void close_cb(uv_handle_t* handle);
		int           status_;
	};
}
}