#include "net.h"
#include "../coroutine.h"
#include "udp_socket.h"
#include "unix_socket.h"
#include "unix_server.h"
#include "tcp_socket.h"
#include "tcp_server.h"
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
		class_unix_socket.add<&unix_socket::connect>("connect");
		class_unix_socket.add<&unix_socket::read>("read");
		class_unix_socket.add<&unix_socket::write>("write");
		class_unix_socket.add<&unix_socket::close>("close");
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
		class_udp_socket.add<&udp_socket::recv>("recv", {
			php::of_string("addr", true, true), // 引用参数
			php::of_string("port", true, true), // 引用参数
		});
		class_udp_socket.add<&udp_socket::send>("send");
		class_udp_socket.add<&udp_socket::close>("close");
		ext.add(std::move(class_udp_socket));

		// 子命名空间
		flame::net::http::init(ext);
		flame::net::fastcgi::init(ext);
	}

}
}
