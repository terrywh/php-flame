#pragma once

namespace net {
	void init(php::extension_entry& extension);
	address addr_from_str(const std::string& addr, bool use_ipv6);
}
