#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "tcp_socket.h"
#include "tcp_server.h"
#include "net.h"

namespace flame {
namespace net {
	tcp_server::tcp_server()
	: svr((uv_tcp_t*)malloc(sizeof(uv_tcp_t)))
	, acc((uv_stream_t*)svr)
	, init_(false)
	, bound_(false) {		
		
	}
	tcp_server::~tcp_server() {
		close();
	}
	php::value tcp_server::handle(php::parameters& params) {
		acc.handle(params[0]);
		return nullptr;
	}
	php::value tcp_server::bind(php::parameters& params) {
		std::string addr = params[0];
		int         port = params[1].to_long(), err;
		struct sockaddr_storage address;
		err = sock_addrfrom(&address, addr.c_str(), port);
		if(err < 0) {
			throw php::exception(uv_strerror(err), err);
		}
		uv_tcp_init_ex(flame::loop, svr, address.ss_family);
		init_ = true;
		// 然后才能进行 SO_REUSEPORT 设置
		sock_reuseport(reinterpret_cast<uv_handle_t*>(svr));
		err = uv_tcp_bind(svr, reinterpret_cast<struct sockaddr*>(&address), 0);
		if(err < 0) {
			throw php::exception(uv_strerror(err), err);
		}
		bound_ = true;
		// 服务器属性
		prop("local_addresss", 14) = addr;
		prop("local_port", 10) = port;
		return nullptr;
	}
	php::value tcp_server::run(php::parameters& params) {
		if(!init_ || !bound_) {
			throw php::exception("failed to run: server hasn't been bound");
		}else{
			acc.run();
			return flame::async();
		}
	}
	php::value tcp_server::close(php::parameters& params) {
		close();
		return nullptr;
	}
	void tcp_server::close() {
		if(svr) {
			acc.close();
			if(init_) uv_close((uv_handle_t*)svr, flame::free_handle_cb);
			else free(svr); // 未做初始化直接 free 即可
			svr = nullptr;
		}
	}
	
}
}
