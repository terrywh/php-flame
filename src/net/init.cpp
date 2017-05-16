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
			sprintf(buffer, "[::1]:%d", port);
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
	void server_socket_reusable(evutil_socket_t fd) {
#ifdef SO_REUSEPORT
		// 服务端需要启用下面选项，以支持更高性能的多进程形式
		int opt = 1;
		if(0 != setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof (opt))) {
			throw php::exception(
				(boost::format("bind failed: %s") % strerror(errno)).str(), errno);
		}
#endif
	}
}
