#include "server.h"

namespace flame {
namespace net {
namespace http {
	php::value server::handle(php::parameters& params) {
		if(params.length() >= 2) { // 两个参数的情况下，设置 path => handle 映射
			std::string path = params[0];
			map_[path] = params[1];
		}else if(params.length() >= 1) { // 单个参数设置默认 handle 
			handle_ = params[0];
		}
		++status_;
		return nullptr;
	}
	void server::accept(uv_stream_t* s) {
		tcp_socket* sock = reinterpret_cast<tcp_socket*>(s->data);
		fiber::start(handle_);
	}
	uv_stream_t* server::create_stream() {
		server_socket* pobj = new server_socket();
		uv_tcp_init(flame::loop, &pobj->socket_);
		pobj->socket_.data = pobj;
		return reinterpret_cast<uv_stream_t*>(&pobj->socket_);
	}
}
}
}
