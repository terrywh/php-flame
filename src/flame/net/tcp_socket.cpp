#include "../coroutine.h"
#include "../../util/sock.h"
#include "tcp_socket.h"

namespace flame {
namespace net {
	tcp_socket::tcp_socket() {
		handler = new handler_t(this);
		uv_tcp_init(flame::loop, &handler->socket);
	}
	tcp_socket::~tcp_socket() {
		handler->close(false); // 析构时 read 过程一定已经停止
	}
	php::value tcp_socket::connect(php::parameters& params) {
		php::string addr = params[0];
		int         port = params[1];
		struct sockaddr_storage address;
		int error = util::sock_addrfrom(&address, addr.c_str(), port);
		if(error != 0) {
			throw php::exception(uv_strerror(error), error);
		}
		connect_request_t* ctx = new connect_request_t {
			.co = coroutine::current,
			.obj = this,
		};
		ctx->req.data = ctx;
		error = uv_tcp_connect(&ctx->req, &handler->socket, reinterpret_cast<struct sockaddr*>(&address), connect_cb);
		if(error != 0) {
			delete ctx;
			throw php::exception(uv_strerror(error), error);
		}
		return flame::async();
	}
	void tcp_socket::connect_cb(uv_connect_t* handle, int error) {
		connect_request_t* ctx = static_cast<connect_request_t*>(handle->data);
		if(error < 0) {
			ctx->co->fail(uv_strerror(error), error);
		}else{
			ctx->obj.native<tcp_socket>()->after_init();
			ctx->co->next();
		}
		delete ctx;
	}
	void tcp_socket::after_init() {
		struct sockaddr_storage address;
		int                     addrlen = sizeof(address);
		uv_tcp_getsockname(&handler->socket,
			reinterpret_cast<struct sockaddr*>(&address),
			&addrlen);
		prop("local_address") = util::sock_addr2str(&address) +
			":" + std::to_string(util::sock_addrport(&address));
		uv_tcp_getpeername(&handler->socket,
			reinterpret_cast<struct sockaddr*>(&address),
			&addrlen);
		prop("remote_address") = util::sock_addr2str(&address) +
			":" + std::to_string(util::sock_addrport(&address));
	}
}
}
