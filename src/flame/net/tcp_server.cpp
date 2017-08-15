#include "../fiber.h"
#include "net.h"
#include "tcp_server.h"
#include "tcp_socket.h"

namespace flame {
namespace net {
	tcp_server::tcp_server()
	: stream_server(reinterpret_cast<uv_stream_t*>(&server_)) {

	}
	php::value tcp_server::handle(php::parameters& params) {
		handle_ = params[0];
		return this;
	}
	php::value tcp_server::bind(php::parameters& params) {
		php::string& addr = params[0];
		int          port = params[1];
		struct sockaddr_storage address;
		int error = addrfrom(&address, addr.c_str(), port);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		// uv_tcp_init_ex 会创建 socket
		uv_tcp_init_ex(flame::loop, &server_, address.ss_family);
		// 然后才能进行 SO_REUSEPORT 设置
		enable_socket_reuseport(reinterpret_cast<uv_handle_t*>(&server_));
		error = uv_tcp_bind(&server_, reinterpret_cast<struct sockaddr*>(&address), 0);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		// 服务器属性
		prop("local_addresss") = addr;
		prop("local_port")     = port;
		return nullptr;
	}
	void tcp_server::accept(uv_stream_t* s) {
		tcp_socket* sock = reinterpret_cast<tcp_socket*>(s->data);
		fiber::start(handle_, sock);
		sock->delref();
	}
	uv_stream_t* tcp_server::create_stream() {
		php::object  sobj = php::object::create<tcp_socket>();
		tcp_socket*  pobj = sobj.native<tcp_socket>();
		uv_tcp_init(flame::loop, &pobj->socket_);
		pobj->socket_.data = pobj;
		pobj->addref(); // 保证 php 对象的生存周期
		return reinterpret_cast<uv_stream_t*>(&pobj->socket_);
	}
}
}
