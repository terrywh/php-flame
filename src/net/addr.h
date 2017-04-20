#pragma once

namespace net {
	class udp_socket;
	class addr_t: public php::class_base {
	public:
		php::value __toString(php::parameters& params);
		php::value port(php::parameters& params);
		php::value host(php::parameters& params);
	private:
		mill_ipaddr addr_;
		unsigned short port_;
		friend class net::udp_socket;
	};
}
