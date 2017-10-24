#pragma once

namespace flame {
class coroutine;
namespace net {
	class unix_server: public php::class_base {
	public:
		unix_server();
		php::value run(php::parameters& params);
		php::value close(php::parameters& params);
		php::value handle(php::parameters& params);
		php::value bind(php::parameters& params);
		// property local_address ""
	protected:
		php::callable handle_;
	private:
		uv_pipe_t     server_;
		coroutine*        co_;
		static void connection_cb(uv_stream_t* server, int error);
	};
}
}
