#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "../log/log.h"
#include "net.h"
#include "udp_socket.h"
#include "udp_packet.h"
#include "unix_socket.h"
#include "unix_server.h"
#include "tcp_socket.h"
#include "tcp_server.h"
#include "http/http.h"
#include "fastcgi/fastcgi.h"

namespace flame {
namespace net {
	void init(php::extension_entry& ext) {
		ext.add<interfaces>("flame\\net\\interfaces");
		// class_udp_socket
		// ------------------------------------
		php::class_entry<udp_socket> class_udp_socket("flame\\net\\udp_socket");
		class_udp_socket.prop({"local_address", std::string("")});
		class_udp_socket.prop({"local_port", 0});
		class_udp_socket.add<&udp_socket::bind>("bind");
		class_udp_socket.add<&udp_socket::recv>("recv");
		class_udp_socket.add<&udp_socket::send>("send");
		class_udp_socket.add<&udp_socket::close>("close");
		ext.add(std::move(class_udp_socket));
		
		php::class_entry<udp_packet> class_udp_packet("flame\\net\\udp_packet");
		class_udp_packet.prop({"payload", nullptr});
		class_udp_packet.prop({"remote_address", std::string("")});
		class_udp_packet.prop({"remote_port", 0});
		class_udp_packet.add<&udp_packet::to_string>("__toString");
		ext.add(std::move(class_udp_packet));
		// class_tcp_socket
		// ------------------------------------
		php::class_entry<tcp_socket> class_tcp_socket("flame\\net\\tcp_socket");
		class_tcp_socket.prop({"local_address", std::string("")});
		class_tcp_socket.prop({"local_port", std::string("")});
		class_tcp_socket.prop({"remote_address", std::string("")});
		class_tcp_socket.prop({"remote_port", 0});
		class_tcp_socket.add<&tcp_socket::connect>("connect");
		class_tcp_socket.add<&tcp_socket::read>("read");
		class_tcp_socket.add<&tcp_socket::read_all>("read_all");
		class_tcp_socket.add<&tcp_socket::write>("write");
		class_tcp_socket.add<&tcp_socket::close>("close");
		ext.add(std::move(class_tcp_socket));
		// class_tcp_server
		// ------------------------------------
		php::class_entry<tcp_server> class_tcp_server("flame\\net\\tcp_server");
		class_tcp_server.prop({"local_address", std::string("")});
		class_tcp_server.prop({"local_port", std::string("")});
		class_tcp_server.add<&tcp_server::bind>("bind");
		class_tcp_server.add<&tcp_server::handle>("handle");
		class_tcp_server.add<&tcp_server::run>("run");
		class_tcp_server.add<&tcp_server::close>("close");
		ext.add(std::move(class_tcp_server));
		// class_unix_socket
		// ------------------------------------
		php::class_entry<unix_socket> class_unix_socket("flame\\net\\unix_socket");
		class_unix_socket.add<&unix_socket::connect>("connect");
		class_unix_socket.add<&unix_socket::read>("read");
		class_unix_socket.add<&unix_socket::read_all>("read_all");
		class_unix_socket.add<&unix_socket::write>("write");
		class_unix_socket.add<&unix_socket::close>("close");
		ext.add(std::move(class_unix_socket));
		// class_unix_server
		// ------------------------------------
		php::class_entry<unix_server> class_unix_server("flame\\net\\unix_server");
		class_unix_server.add<&unix_server::bind>("bind");
		class_unix_server.add<&unix_server::handle>("handle");
		class_unix_server.add<&unix_server::run>("run");
		class_unix_server.add<&unix_server::close>("close");
		ext.add(std::move(class_unix_server));
		// 子命名空间
		// --------------------------------------
		flame::net::http::init(ext);
		flame::net::fastcgi::init(ext);
	}
	
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
			log::default_logger->write(fmt::format("(WARN) unable to enable SO_REUSEPORT: ({0}) {1}", errno, strerror(errno)));
		}
	#endif
	}
	
	php::value interfaces(php::parameters& params) {
		uv_interface_address_t* addrs;
		int count;		
		php::array ifaces(2);
		char   addr[32];
		size_t size;
		uv_interface_addresses(&addrs, &count);
		for(int i=0;i<count;++i) {
			php::array_item_assoc face = ifaces.at(addrs[i].name);
			if(face.is_undefined()) {
				face = php::array(4);
			}
			php::array  iaddr(2);
			size = snprintf(addr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
				static_cast<unsigned char>(addrs[i].phys_addr[0]),
				static_cast<unsigned char>(addrs[i].phys_addr[1]),
				static_cast<unsigned char>(addrs[i].phys_addr[2]),
				static_cast<unsigned char>(addrs[i].phys_addr[3]),
				static_cast<unsigned char>(addrs[i].phys_addr[4]),
				static_cast<unsigned char>(addrs[i].phys_addr[5]));
			iaddr.at("mac",3) = php::string(addr, size);
			iaddr.at("internal",8) = addrs[i].is_internal ? php::BOOL_TRUE : php::BOOL_FALSE;
			if(addrs[i].address.address4.sin_family == AF_INET) {
				uv_ip4_name(&addrs[i].address.address4, addr, sizeof(addr));
				iaddr.at("address",7) = php::string(addr);
				uv_ip4_name(&addrs[i].netmask.netmask4, addr, sizeof(addr));
				iaddr.at("netmask",7) = php::string(addr);
				iaddr.at("family",6) = php::string("IPv4",4);
			}else if(addrs[i].address.address4.sin_family == AF_INET6) {
				uv_ip6_name(&addrs[i].address.address6, addr, sizeof(addr));
				iaddr.at("address",7) = php::string(addr);
				uv_ip6_name(&addrs[i].netmask.netmask6, addr, sizeof(addr));
				iaddr.at("netmask",7) = php::string(addr);
				iaddr.at("family",6) = php::string("IPv6",4);
				iaddr.at("scopeid",7) = addrs[i].address.address6.sin6_scope_id;
			}else{
				php::string unknown("<unknown>",9);
				iaddr.at("address",7) = unknown;
				iaddr.at("family",6) = unknown;
			}
			php::array iface = face;
			iface.at(iface.length()) = iaddr;
		}
		uv_free_interface_addresses(addrs, count);
		return std::move(ifaces);
	}
}
}
