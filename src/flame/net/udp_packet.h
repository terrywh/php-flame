#pragma once

namespace flame {
namespace net {
	class udp_packet: public php::class_base {
	public:
		void init(const php::string& data, const struct sockaddr_storage* addr);
		php::value to_string(php::parameters& params);
		// property data
		// property remote_address
		// property remote_port
	private:
	};
}
}
