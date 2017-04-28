#include "../vendor.h"
#include "init.h"
#include "addr.h"
#include "udp_socket.h"
#include "tcp_socket.h"
#include "tcp_server.h"
#include "http/init.h"

namespace net {
	void init(php::extension_entry& extension) {
		// mill_address
		// ---------------------------------------------------------------------
		php::class_entry<addr_t> mill_addr("mill\\addr");
		mill_addr.add<&addr_t::host>("host");
		mill_addr.add<&addr_t::port>("port");
		mill_addr.add<&addr_t::__toString>("__toString");
		extension.add(std::move(mill_addr));
		// mill_udp_socket
		// ---------------------------------------------------------------------
		php::class_entry<udp_socket> mill_udp_socket("mill\\net\\udp_socket");
		mill_udp_socket.add<&udp_socket::__construct>("__construct", {
			php::of_string("addr"),
			php::of_integer("port"),
		});
		mill_udp_socket.add<&udp_socket::__destruct>("__destruct");
		mill_udp_socket.add<&udp_socket::remote_addr>("remote_addr");
		mill_udp_socket.add<&udp_socket::close>("close");
		mill_udp_socket.add<&udp_socket::recv>("recv");
		mill_udp_socket.add<&udp_socket::send>("send", {
			php::of_string("data"),
			php::of_string("host"),
			php::of_integer("port"),
		});
		mill_udp_socket.add(php::property_entry("local_port", nullptr));
		mill_udp_socket.add(php::property_entry("closed", true));
		extension.add(std::move(mill_udp_socket));
		// mill_tcp_socket
		// ---------------------------------------------------------------------
		php::class_entry<tcp_socket> mill_tcp_socket("mill\\net\\tcp_socket");
		mill_tcp_socket.add<&tcp_socket::__construct>("__construct", {
			php::of_string("addr"),
			php::of_integer("port"),
			php::of_integer("deadline"),
		});
		mill_tcp_socket.add<&tcp_socket::__destruct>("__destruct");
		mill_tcp_socket.add<&tcp_socket::remote_addr>("remote_addr");
		mill_tcp_socket.add<&tcp_socket::close>("close");
		mill_tcp_socket.add<&tcp_socket::recv>("recv", {
			php::of_mixed("stop"), // integer or string
			php::of_integer("deadline"),
		});
		mill_tcp_socket.add<&tcp_socket::send>("send", {
			php::of_string("data"),
			php::of_integer("deadline"),
		});
		mill_tcp_socket.add<&tcp_socket::send_buffer>("send_buffer", {
			php::of_string("data"),
			php::of_integer("deadline"),
		});
		mill_tcp_socket.add<&tcp_socket::flush>("flush_buffer", {
			php::of_integer("deadline"),
		});
		mill_tcp_socket.add(php::property_entry("closed", true));
		extension.add(std::move(mill_tcp_socket));
		// mill_tcp_socket
		// ---------------------------------------------------------------------
		php::class_entry<tcp_server> mill_tcp_server("mill\\net\\tcp_server");
		mill_tcp_server.add<&tcp_server::__construct>("__construct", {
			php::of_string("addr"),
			php::of_integer("port"),
		});
		mill_tcp_server.add<&tcp_server::__destruct>("__destruct");
		mill_tcp_server.add<&tcp_server::close>("close");
		mill_tcp_server.add<&tcp_server::accept>("accept");
		mill_tcp_server.add(php::property_entry("local_port", nullptr));
		mill_tcp_server.add(php::property_entry("closed", true));
		extension.add(std::move(mill_tcp_server));

		http::init(extension);
	}
}
