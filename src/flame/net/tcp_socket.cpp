#include "../fiber.h"
#include "net.h"
#include "tcp_socket.h"

namespace flame {
namespace net {
	tcp_socket::tcp_socket()
	: stream_socket(reinterpret_cast<uv_stream_t*>(&socket_)) {
		uv_tcp_init(flame::loop, &socket_);
		// socket_.data = this;
	}

	php::value tcp_socket::connect(php::parameters& params) {
		php::string addr = params[0];
		int         port = params[1];
		struct sockaddr_storage address;
		int error = addrfrom(&address, addr.c_str(), port);
		if(error != 0) {
			throw php::exception(uv_strerror(error), error);
		}
		uv_connect_t* req = new uv_connect_t;
		req->data = flame::this_fiber()->push(this);
		error = uv_tcp_connect(req, &socket_, reinterpret_cast<struct sockaddr*>(&address), connect_cb);
		if(error != 0) {
			delete req;
			flame::this_fiber()->throw_exception(uv_strerror(error), error);
		}
		return flame::async();
	}
	void tcp_socket::connect_cb(uv_connect_t* req, int error) {
		flame::fiber*  f = reinterpret_cast<flame::fiber*>(req->data);
		tcp_socket* self = f->context<tcp_socket>();
		delete req;

		if(error < 0) {
			f->next(php::make_exception(uv_strerror(error), error));
		}else{
			self->init_prop();
			f->next();
		}
	}
	void tcp_socket::init_prop() {
		struct sockaddr_storage address;
		int                     addrlen = sizeof(address);
		uv_tcp_getsockname(&socket_,
			reinterpret_cast<struct sockaddr*>(&address),
			&addrlen);
		prop("local_address") = addr2str(&address) + ":" + std::to_string(addrport(&address));
		uv_tcp_getpeername(&socket_,
			reinterpret_cast<struct sockaddr*>(&address),
			&addrlen);
		prop("remote_address") = addr2str(&address) + ":" + std::to_string(addrport(&address));
	}
}
}
