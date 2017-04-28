#include "../vendor.h"
#include "udp_socket.h"

namespace net {
	void init(php::extension_entry& extension) {
		udp_socket::init(extension);
	}
}
