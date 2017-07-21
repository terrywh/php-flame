#include "net.h"
#include "stream_socket.h"
#include "tcp_socket.h"
#include "tcp_server.h"
#include "http_client.h"

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
		
		// class_http_client
		// ------------------------------------
		if(curl_global_init(CURL_GLOBAL_ALL)) {
			fprintf(stderr, "Could not init curl\n");
			return ;
		}

		ext.add<http_get>("flame\\net\\http_get");
		ext.add<http_post>("flame\\net\\http_post");
		ext.add<http_put>("flame\\net\\http_put");

		php::class_entry<http_request> class_http_request("flame\\net\\http_request");
		class_http_request.add(php::property_entry("url", ""));
		class_http_request.add(php::property_entry("method", ""));
		class_http_request.add(php::property_entry("timeout", 10));
		class_http_request.add(php::property_entry("header", nullptr));
		class_http_request.add<&http_request::__construct>("__construct");
		ext.add(std::move(class_http_request));

		php::class_entry<http_client> class_http_client("flame\\net\\http_client");
		class_http_client.add<&http_client::exec>("exec");
		class_http_client.add<&http_client::debug>("debug");
		ext.add(std::move(class_http_client));
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


