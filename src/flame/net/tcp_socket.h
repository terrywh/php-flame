#pragma once
#include "client_impl.h"

namespace flame {
namespace net {
	class tcp_socket: public php::class_base {
	public:
		typedef client_impl<uv_tcp_t, tcp_socket> impl_t;
		tcp_socket();
		~tcp_socket();
		php::value connect(php::parameters& params);
		inline php::value read(php::parameters& params) {
			return impl->read(params);
		}
		inline php::value write(php::parameters& params) {
			return impl->write(params);
		}
		inline php::value close(php::parameters& params) {
			return impl->close(params);
		}
		// property local_address ""
		// property remote_address ""
		impl_t* impl; // 由于异步关闭需要等待 close_cb 才能进行回收
		void after_init();
	private:
		static void connect_cb(uv_connect_t* req, int status);

		void init_prop();
	};
}
}
