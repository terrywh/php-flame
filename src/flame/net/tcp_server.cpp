#include "../coroutine.h"
#include "../../util/sock.h"
#include "tcp_socket.h"
#include "tcp_server.h"

namespace flame {
namespace net {
	tcp_server::tcp_server() {
		handler = new handler_t(this);
		uv_tcp_init(flame::loop, &handler->server);
	}
	tcp_server::~tcp_server() {
		handler->close(false); // 析构时运行过程一定已经停止
	}
	php::value tcp_server::bind(php::parameters& params) {
		std::string addr = params[0];
		int         port = params[1];
		struct sockaddr_storage address;
		int error = util::sock_addrfrom(&address, addr.c_str(), port);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		// uv_tcp_init_ex 会创建 socket
		uv_tcp_init_ex(flame::loop, &handler->server, address.ss_family);
		// 然后才能进行 SO_REUSEPORT 设置
		util::sock_reuseport(reinterpret_cast<uv_handle_t*>(&handler->server));
		error = uv_tcp_bind(&handler->server,
			reinterpret_cast<struct sockaddr*>(&address), 0);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		// 服务器属性
		prop("local_addresss") = addr + ":" + std::to_string(port);
		return nullptr;
	}
}
}
