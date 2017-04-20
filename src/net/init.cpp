#include "../vendor.h"
#include "init.h"
#include "addr.h"
#include "udp_socket.h"

namespace net {
	void init(php::extension_entry& extension) {
		// mill_address
		php::class_entry<addr_t> mill_addr("mill\\addr");
		mill_addr.add<&addr_t::host>("host");
		mill_addr.add<&addr_t::port>("port");
		mill_addr.add<&addr_t::__toString>("__toString");
		extension.add(std::move(mill_addr));
		// mill_udp_socket
		php::class_entry<udp_socket> mill_udp_socket("mill\\net\\udp_socket");
		mill_udp_socket.add<&udp_socket::__construct>("__construct", {
			php::of_string("addr"),
			php::of_integer("port"),
		});
		mill_udp_socket.add<&udp_socket::local_addr>("local_addr");
		mill_udp_socket.add<&udp_socket::remote_addr>("remote_addr");
		mill_udp_socket.add<&udp_socket::close>("close");
		mill_udp_socket.add<&udp_socket::recv>("recv");
		mill_udp_socket.add<&udp_socket::send>("send", {
			php::of_string("data"),
			php::of_string("host"),
			php::of_integer("port"),
		});
		extension.add(std::move(mill_udp_socket));
	}
}
