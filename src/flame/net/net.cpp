#include "net.h"
#include "tcp_socket.h"
#include "tcp_server.h"

namespace flame {
namespace net {
	void init(php::extension_entry& ext) {
		// class_tcp_socket
		// ------------------------------------
		php::class_entry<tcp_socket> class_tcp_socket("flame\\net\\tcp_socket");
		class_tcp_socket.add(php::property_entry("local_address", std::string("")));
		class_tcp_socket.add(php::property_entry("local_port", 0));
		class_tcp_socket.add(php::property_entry("remote_address", std::string("")));
		class_tcp_socket.add(php::property_entry("remote_port", 0));
		class_tcp_socket.add<&tcp_socket::connect>("connect");
		class_tcp_socket.add<&tcp_socket::read>("read");
		class_tcp_socket.add<&tcp_socket::write>("write");
		class_tcp_socket.add<&tcp_socket::__destruct>("__destruct");
		ext.add(std::move(class_tcp_socket));
		// class_tcp_server
		// ------------------------------------
		php::class_entry<tcp_server> class_tcp_server("flame\\net\\tcp_server");
		class_tcp_server.add(php::property_entry("local_address", std::string("")));
		class_tcp_server.add(php::property_entry("local_port", 0));
		class_tcp_server.add<&tcp_server::bind>("bind");
		class_tcp_server.add<&tcp_server::handle>("handle");
		class_tcp_server.add<&tcp_server::run>("run");
		class_tcp_server.add<&tcp_server::close>("close");
		class_tcp_server.add<&tcp_server::__destruct>("__destruct");
		ext.add(std::move(class_tcp_server));
	}
	php::string addr2str(const struct sockaddr_storage& addr) {
		php::string str(64);
		std::memset(str.data(), 0, 64);
		switch(addr.ss_family) {
			case AF_INET:
				uv_ip4_name((struct sockaddr_in*)&addr, str.data(), str.length());
			break;
			case AF_INET6:
				uv_ip6_name((struct sockaddr_in6*)&addr, str.data(), str.length());
			break;
		}
		return std::move(str);
	}
	uint16_t addr2int(const struct sockaddr_storage& addr) {
		switch(addr.ss_family) {
		case AF_INET:
			return ntohs(reinterpret_cast<const struct sockaddr_in&>(addr).sin_port);
		case AF_INET6:
			return ntohs(reinterpret_cast<const struct sockaddr_in6&>(addr).sin6_port);
		}
	}
	int addrfrom(struct sockaddr_storage& addr, const char* str, uint16_t port) {
		int error = uv_ip6_addr(str, port, reinterpret_cast<struct sockaddr_in6*>(&addr));
		if(error != 0) {
			error = uv_ip4_addr(str, port, reinterpret_cast<struct sockaddr_in*>(&addr));
		}
		return error;
	}
}	
}


