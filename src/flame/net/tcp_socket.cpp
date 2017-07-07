#include "tcp_socket.h"

namespace flame {
namespace net {
	tcp_socket::tcp_socket() {
		uv_tcp_init(uv_default_loop(), &socket_);
		socket_.data = this;
	}
	void tcp_socket::getaddrinfo_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {

		delete req;
	}
	php::value tcp_socket::connect(php::parameters& params) {
		php::string addr = params[0];
		php::string port = params[1];

		uv_getaddrinfo_t* req = new uv_getaddrinfo_t();
		req->data = this;
		if(0 > uv_getaddrinfo(uv_default_loop(), req, getaddrinfo_cb, addr.c_str(), port.c_str(), nullptr)) {
			reject(php::make_exception(strerror(errno), errno));
			return future_;
		}
			
		// uv_ip6_addr()
		// uv_tcp_connect(req, &socket_, );
		return future_;
	}
	php::value tcp_socket::read(php::parameters& params) {}
	php::value tcp_socket::read_len(php::parameters& params) {}
	php::value tcp_socket::read_sep(php::parameters& params) {}
	php::value tcp_socket::write(php::parameters& params) {}

}
}