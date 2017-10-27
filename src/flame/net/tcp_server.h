#pragma once
#include "server_impl.h"

namespace flame {
class coroutine;
namespace net {
	class tcp_server: public php::class_base {
	public:
		typedef server_impl<uv_tcp_t, tcp_server, tcp_socket> impl_t;
		tcp_server();
		~tcp_server();
		php::value bind(php::parameters& params);
		inline php::value handle(php::parameters& params) {
			return impl->handle(params);
		}
		inline php::value run(php::parameters& params) {
			return impl->run(params);
		}
		inline php::value close(php::parameters& params) {
			return impl->close(params);
		}
		// property local_address ""
		impl_t* impl; // 由于异步关闭需要等待 close_cb 才能进行回收
	private:
		static void connection_cb(uv_stream_t* server, int error);
	};
}
}
