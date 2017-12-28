#pragma once
#include "server_acceptor.h"

namespace flame {
namespace net {
	class tcp_server: public php::class_base {
	public:
		tcp_server();
		~tcp_server();
		php::value bind(php::parameters& params);
		php::value handle(php::parameters& params);
		php::value run(php::parameters& params);
		php::value close(php::parameters& params);
		void close();
		// property local_address ""
		// 由于异步关闭需要等待 close_cb 才能进行回收
		uv_tcp_t*                   svr; 
		server_acceptor<tcp_socket> acc;
	private:
		bool init_, bound_;
	};
}
}
