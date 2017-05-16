#pragma once

namespace net {
	void init(php::extension_entry& extension);
	void parse_addr_port(const char* address, int port, struct sockaddr* addr, int *size_of_addr);
	void parse_addr(int af, struct sockaddr* addr, char* dst, size_t& len);
	void parse_mili(int mili, struct timeval* to);
}
