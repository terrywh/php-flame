#pragma once

namespace net {
	class udp_socket: public php::class_base {
	public:
		udp_socket():closed_(true) {}
		php::value __construct(php::parameters& params);
		php::value __destruct(php::parameters& params);
		php::value remote_addr(php::parameters& params);
		php::value close(php::parameters& params);
		php::value recv(php::parameters& params);
		php::value send(php::parameters& params);
	private:
		mill_ipaddr  local_addr_;
		mill_ipaddr  remote_addr_;
		mill_udpsock socket_;
		bool         closed_;
		char         buffer_[64*1024];
	};
}
