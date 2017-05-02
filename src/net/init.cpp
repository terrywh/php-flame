#include "../vendor.h"
#include "udp_socket.h"
#include "tcp_socket.h"
#include "tcp_server.h"

namespace net {
	void init(php::extension_entry& extension) {
		udp_socket::init(extension);
		tcp_socket::init(extension);
		tcp_server::init(extension);
	}
	address addr_from_str(const std::string& addr, bool use_ipv6) {
		auto addr_ = address::from_string(addr);
		if(use_ipv6 && addr_.is_v4()) {
			try{
				addr_ = boost::asio::ip::address_v6::v4_mapped(addr_.to_v4());
			}catch(std::bad_cast& ex) {
				throw php::exception("illegal address: using IPv6 but IPv4 addr given", 0);
			}
		}else if(!use_ipv6 && addr_.is_v6()) {
			try{
				addr_ = addr_.to_v6().to_v4();
			}catch(std::bad_cast& ex) {
				throw php::exception("illegal address: using IPv4 but IPv6 addr given", 0);
			}
		}
		return addr_;
	}
}
