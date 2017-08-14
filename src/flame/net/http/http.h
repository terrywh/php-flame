#pragma once

namespace flame {
namespace net {
namespace http {
	void init(php::extension_entry& ext);
	extern std::map<int, std::string> status_mapper;
}
}
}
