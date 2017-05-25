#include "../vendor.h"
#include "../core.h"
#include "init.h"
#include "udp_socket.h"
#include "tcp_socket.h"
#include "tcp_server.h"

namespace net {
	void init(php::extension_entry& extension) {
		udp_socket::init(extension);
		tcp_socket::init(extension);
		tcp_server::init(extension);
	}

	void parse_addr_port(const char* address, int port, struct sockaddr* addr, int *size_of_addr) {
		char buffer[64];
		if(std::strstr(address, ":") != nullptr) { // IPv6
			sprintf(buffer, "[%s]:%d", address, port);
		}else if(std::strlen(address) > 0) { // IPv4
			sprintf(buffer, "%s:%d", address, port);
		}else{ // 默认按 IPv6 兼容 IPv4 地址
			sprintf(buffer, "[::]:%d", port);
		}
		if(-1 == evutil_parse_sockaddr_port(buffer, addr, size_of_addr)) {
			throw php::exception("parse failed: illegal address");
		}
	}

	void parse_addr(int af, struct sockaddr* addr, char* dst, size_t& len) {
		if(af == AF_INET) {
			sockaddr_in* r = (sockaddr_in*)addr;
			evutil_inet_ntop(af, &r->sin_addr, dst, len);
			len = std::strlen(dst);
		}else if(af == AF_INET) {
			sockaddr_in6* r = (sockaddr_in6*)addr;
			evutil_inet_ntop(af, &r->sin6_addr, dst, len);
			len = std::strlen(dst);
		}else {
			assert(0);
		}
	}

	void parse_mili(int mili, struct timeval* to) {
		to->tv_sec = mili / 1000;
		to->tv_usec = mili * 1000 % 1000000;
	}

	evutil_socket_t create_socket(int af, int type, int proto, bool svr) {
		evutil_socket_t socket_ = socket(af, type, proto);
		if(socket_ < 0) {
			throw php::exception(
				(boost::format("create socket failed: %s") % strerror(errno)).str(),
				errno
			);
		}
		evutil_make_socket_nonblocking(socket_);
		if(svr) {
			evutil_make_listen_socket_reuseable(socket_);
			evutil_make_listen_socket_reuseable_port(socket_);
		}
		if(af == AF_INET6) {
			int opt = 0;
			if(-1 == setsockopt(socket_, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt))) {
				throw php::exception(
					(boost::format("create socket failed: %s") % strerror(errno)).str(),
					errno
				);
			}
		}
		return socket_;
	}
}
