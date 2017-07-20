#pragma once 

namespace flame {
namespace net {
	void init(php::extension_entry& ext);
	php::string addr2str(const struct sockaddr_storage& addr);
	uint16_t    addr2int(const struct sockaddr_storage& addr);
	int addrfrom(struct sockaddr_storage& addr, const char* str, uint16_t port);
}	
}
