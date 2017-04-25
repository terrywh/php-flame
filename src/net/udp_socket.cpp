#include "../vendor.h"
#include "addr.h"
#include "udp_socket.h"
#include <iostream>

namespace net {

	php::value udp_socket::__construct(php::parameters& params) {
		local_addr_ = mill_iplocal(params[0], params[1], 0);
		if(errno != 0) {
			throw php::exception("failed to create udp socket (iplocal)", errno);
		}
		socket_ = mill_udplisten(local_addr_);
		if(errno != 0) {
			throw php::exception("failed to create udp socket (udplisten)", errno);
		}
		prop("local_port", mill_udpport(socket_));
		closed_ = false;
		prop("closed", closed_);
		return nullptr;
	}

	php::value udp_socket::__destruct(php::parameters& params) {
		if(!closed_) {
			closed_ = true;
			mill_udpclose(socket_);
		}
		return nullptr;
	}

	php::value udp_socket::remote_addr(php::parameters& params) {
		php::value addr = php::value::object<net::addr_t>();
		addr.native<net::addr_t>()->addr_ = remote_addr_;
		addr.native<net::addr_t>()->port_ = mill_ipport(remote_addr_);
		return std::move(addr);
	}

	php::value udp_socket::recv(php::parameters& params) {
		std::int64_t dead = mill_now();
AGAIN:
		std::size_t len = mill_udprecv(socket_, &remote_addr_, buffer_, sizeof(buffer_), dead += 1000);
		if(errno != 0) {
			if(errno != ETIMEDOUT) { // 读取到达1秒超时，不是错误
				throw php::exception("failed to recv on udp socket", errno);
			}
			goto AGAIN;
		}
		return php::value(buffer_, len, false);
	}

	php::value udp_socket::send(php::parameters& params) {
		std::int64_t dead = mill_now();
	 	zend_string* data = params[0];
		const char*  addr = params[1];
		int          port = params[2];
		mill_ipaddr  addr_= mill_ipremote(addr, port, 0, dead + 1000);
		mill_udpsend(socket_, addr_, data->val, data->len);
		if(errno != 0) {
			std::string message("failed to recv on udp socket: ");
			message.append(strerror(errno));
			php::warn(message);
		}
		return nullptr;
	}

	php::value udp_socket::close(php::parameters& params) {
		if(!closed_) {
			closed_ = true;
			prop("closed", closed_);
			mill_udpclose(socket_);
		}
		return nullptr;
	}
}
