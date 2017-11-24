#include "../coroutine.h"
#include "../../util/sock.h"
#include "tcp_socket.h"

namespace flame {
namespace net {
	tcp_socket::tcp_socket() {
		impl = new impl_t(this);
		uv_tcp_init(flame::loop, &impl->stream);
	}
	tcp_socket::~tcp_socket() {
		impl->close(false); // 析构时 read 过程一定已经停止
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
			coroutine::current, this
		};
		ctx->req.data = ctx;
		error = uv_tcp_connect(&ctx->req, &impl->stream, reinterpret_cast<struct sockaddr*>(&address), connect_cb);
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
			static_cast<php::object&>(ctx->obj).native<tcp_socket>()->after_init();
			ctx->co->next();
		}
		delete ctx;
	}
	void tcp_socket::after_init() {
		struct sockaddr_storage address;
		int                     addrlen = sizeof(address);
		uv_tcp_getsockname(&impl->stream,
			reinterpret_cast<struct sockaddr*>(&address),
			&addrlen);
		prop("local_address") = util::sock_addr2str(&address) +
			":" + std::to_string(util::sock_addrport(&address));
		uv_tcp_getpeername(&impl->stream,
			reinterpret_cast<struct sockaddr*>(&address),
			&addrlen);
		prop("remote_address") = util::sock_addr2str(&address) +
			":" + std::to_string(util::sock_addrport(&address));
	}
}
}
