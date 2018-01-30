#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "net.h"
#include "udp_packet.h"

namespace flame {
namespace net {
	void udp_packet::init(const php::string& data, const struct sockaddr_storage* addr) {
		prop("payload", 7) = data;
		prop("remote_address", 14) = sock_addr2str(addr);
		prop("remote_port", 11) = sock_addrport(addr);
	}
	php::value udp_packet::to_string(php::parameters& params) {
		return prop("payload", 7);
	}
}
}
