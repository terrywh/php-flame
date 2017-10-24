#include "sock.h"
namespace util {
	std::string sock_addr2str(const struct sockaddr_storage* addr) {
		char str[64];
		std::memset(str, 0, 64);
		switch(addr->ss_family) {
		case AF_INET:
			uv_ip4_name((struct sockaddr_in*)addr, str, sizeof(str));
		break;
		case AF_INET6:
			uv_ip6_name((struct sockaddr_in6*)addr, str, sizeof(str));
		break;
		}
		return std::string(str);
	}
	uint16_t sock_addrport(const struct sockaddr_storage* addr) {
		switch(addr->ss_family) {
		case AF_INET:
			return ntohs(reinterpret_cast<const struct sockaddr_in*>(addr)->sin_port);
		case AF_INET6:
			return ntohs(reinterpret_cast<const struct sockaddr_in6*>(addr)->sin6_port);
		}
	}
	int sock_addrfrom(struct sockaddr_storage* addr, const char* str, uint16_t port) {
		int error = uv_ip6_addr(str, port, reinterpret_cast<struct sockaddr_in6*>(addr));
		if(error != 0) {
			error = uv_ip4_addr(str, port, reinterpret_cast<struct sockaddr_in*>(addr));
		}
		return error;
	}
	void sock_reuseport(uv_handle_t* h) {
	#ifdef SO_REUSEPORT
		uv_os_fd_t fd;
		uv_fileno(h, &fd);
		int opt = 1;
		if(-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
			php::warn("failed to enable SO_REUSEPORT:", strerror(errno));
		}
	#endif
	}
}
