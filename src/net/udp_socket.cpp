#include "../vendor.h"
#include "addr.h"
#include "udp_socket.h"
#include <iostream>

namespace net {

	php::value udp_socket::__construct(php::parameters& params) {
		std::cout << "type:" << (int)(params[1]) << std::endl;
		local_addr_ = mill_iplocal(params[0], static_cast<int>(params[1]), 0);
		if(errno != 0) {
			throw php::exception("failed to create udp socket: mill_iplocal");
		}
		socket_ = mill_udplisten(local_addr_);
		if(errno != 0) {
			throw php::exception("failed to create udp socket: mill_udplisten");
		}
		return nullptr;
	}

	php::value udp_socket::__destruct(php::parameters& params) {
		return close(params);
	}

	php::value udp_socket::local_addr(php::parameters& params) {
		php::value addr = php::value::object<net::addr_t>();
		addr.native<net::addr_t>()->addr_ = local_addr_;
		addr.native<net::addr_t>()->port_ = mill_udpport(socket_);
		return std::move(addr);
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
				std::string message("failed to recv on udp socket: ");
				message.append(strerror(errno));
				throw php::exception(message);
			}
			goto AGAIN;
		}
		return php::value(buffer_, 0, false);
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
	}

	php::value udp_socket::close(php::parameters& params) {
		mill_udpclose(socket_);
		return nullptr;
	}
}
