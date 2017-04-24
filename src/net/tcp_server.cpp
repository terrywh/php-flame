#include "../vendor.h"
#include "tcp_socket.h"
#include "tcp_server.h"

namespace net {
	php::value tcp_server::__construct(php::parameters& params) {
		local_addr_ = mill_iplocal(params[0], params[1], 0);
		if(errno != 0) {
			throw php::exception("failed to create tcp server (iplocal)", errno);
		}
		server_ = mill_tcplisten(local_addr_, 2048);
		if(errno != 0) {
			throw php::exception("failed to create tcp server (tcplisten)", errno);
		}
		prop("local_port", mill_tcpport(server_));
		closed_ = false;
		prop("closed", closed_);
		return nullptr;
	}
	php::value tcp_server::__destruct(php::parameters& params) {
		return close(params);
	}
	php::value tcp_server::accept(php::parameters& params) {
		std::int64_t dead = -1;
		if(params.length()>0) {
			dead = mill_now() + static_cast<std::int64_t>(params[0]);
		}
		php::value  sock_= php::value::object<tcp_socket>();
		tcp_socket* sock = sock_.native<tcp_socket>();
		sock->socket_ = mill_tcpaccept(server_, dead);
		if(errno != 0) {
			throw php::exception("failed to accept tcp socket", errno);
		}
		sock->remote_addr_ = mill_tcpaddr(sock->socket_);
		sock->closed_ = false;
		sock->prop("closed", sock->closed_);
		return std::move(sock_);
	}
	php::value tcp_server::close(php::parameters& params) {
		if(!closed_) {
			closed_ = true;
			prop("closed", closed_);
			mill_tcpclose(server_);
		}
		return nullptr;
	}
}
