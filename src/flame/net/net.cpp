#include "net.h"
#include "tcp_socket.h"
#include "tcp_server.h"
#include "unix_socket.h"
#include "unix_server.h"
#include "udp_socket.h"

#include "http/http.h"
#include "fastcgi/fastcgi.h"

namespace flame {
namespace net {
	void init(php::extension_entry& ext) {
		// class_tcp_socket
		// ------------------------------------
		php::class_entry<tcp_socket> class_tcp_socket("flame\\net\\tcp_socket");
		class_tcp_socket.add(php::property_entry("local_address", std::string("")));
		class_tcp_socket.add(php::property_entry("remote_address", std::string("")));
		class_tcp_socket.add<&tcp_socket::connect>("connect");
		class_tcp_socket.add<&tcp_socket::read>("read");
		class_tcp_socket.add<&tcp_socket::write>("write");
		class_tcp_socket.add<&tcp_socket::write>("close");
		ext.add(std::move(class_tcp_socket));
		// class_tcp_server
		// ------------------------------------
		php::class_entry<tcp_server> class_tcp_server("flame\\net\\tcp_server");
		class_tcp_server.add(php::property_entry("local_address", std::string("")));
		class_tcp_server.add<&tcp_server::bind>("bind");
		class_tcp_server.add<&tcp_server::handle>("handle");
		class_tcp_server.add<&tcp_server::run>("run");
		class_tcp_server.add<&tcp_server::close>("close");
		ext.add(std::move(class_tcp_server));
		// class_unix_socket
		// ------------------------------------
		php::class_entry<unix_socket> class_unix_socket("flame\\net\\unix_socket");
		class_unix_socket.add(php::property_entry("remote_address", std::string("")));
		class_unix_socket.add<&unix_socket::connect>("connect");
		class_unix_socket.add<&unix_socket::read>("read");
		class_unix_socket.add<&unix_socket::write>("write");
		ext.add(std::move(class_unix_socket));
		// class_unix_server
		// ------------------------------------
		php::class_entry<unix_server> class_unix_server("flame\\net\\unix_server");
		class_unix_server.add(php::property_entry("local_address", std::string("")));
		class_unix_server.add<&unix_server::bind>("bind");
		class_unix_server.add<&unix_server::handle>("handle");
		class_unix_server.add<&unix_server::run>("run");
		class_unix_server.add<&unix_server::close>("close");
		ext.add(std::move(class_unix_server));
		// class_udp_socket
		// ------------------------------------
		php::class_entry<udp_socket> class_udp_socket("flame\\net\\udp_socket");
		class_udp_socket.add(php::property_entry("local_address", std::string("")));
		class_udp_socket.add<&udp_socket::bind>("bind");
		class_udp_socket.add<&udp_socket::recv_from>("recv_from", {
			php::of_string("addr", true, true), // 引用参数
			php::of_string("port", true, true),
		});
		class_udp_socket.add<&udp_socket::send_to>("send_to");
		class_udp_socket.add<&udp_socket::close>("close");
		ext.add(std::move(class_udp_socket));

		// 子命名空间
		flame::net::http::init(ext);
		flame::net::fastcgi::init(ext);
	}
	std::string addr2str(const struct sockaddr_storage* addr) {
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
	uint16_t addrport(const struct sockaddr_storage* addr) {
		switch(addr->ss_family) {
		case AF_INET:
			return ntohs(reinterpret_cast<const struct sockaddr_in*>(addr)->sin_port);
		case AF_INET6:
			return ntohs(reinterpret_cast<const struct sockaddr_in6*>(addr)->sin6_port);
		}
	}
	int addrfrom(struct sockaddr_storage* addr, const char* str, uint16_t port) {
		int error = uv_ip6_addr(str, port, reinterpret_cast<struct sockaddr_in6*>(addr));
		if(error != 0) {
			error = uv_ip4_addr(str, port, reinterpret_cast<struct sockaddr_in*>(addr));
		}
		return error;
	}
	void enable_socket_reuseport(uv_handle_t* h) {
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
}
