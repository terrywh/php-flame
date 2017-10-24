#pragma once

namespace util {
	std::string sock_addr2str(const struct sockaddr_storage* addr);
	uint16_t    sock_addrport(const struct sockaddr_storage* addr);
	int         sock_addrfrom(struct sockaddr_storage* addr, const char* str, uint16_t port);
	void        sock_reuseport(uv_handle_t* h);
}
