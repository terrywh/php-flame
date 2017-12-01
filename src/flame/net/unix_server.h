#pragma once
#include "server_acceptor.h"

namespace flame {
namespace net {
	class unix_socket;
	class unix_server: public php::class_base {
	public:
		unix_server();
		~unix_server();
		php::value bind(php::parameters& params);
		php::value handle(php::parameters& params);
		php::value run(php::parameters& params);
		php::value close(php::parameters& params);
		void close();
		// 由于异步关闭需要等待 close_cb 才能进行回收
		uv_pipe_t*                   svr; 
		server_acceptor<unix_socket> acc;
	private:
		bool bound_;
	};
}
}
