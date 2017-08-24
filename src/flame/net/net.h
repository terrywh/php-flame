#pragma once

namespace flame {
namespace net {
	void init(php::extension_entry& ext);
	std::string addr2str(const struct sockaddr_storage* addr);
	uint16_t    addrport(const struct sockaddr_storage* addr);
	int addrfrom(struct sockaddr_storage* addr, const char* str, uint16_t port);
	void enable_socket_reuseport(uv_handle_t* h);
}
}
