#pragma once

namespace flame {
namespace time {
	void    init(php::extension_entry& ext);
	int64_t now();
	const char* datetime();
}
}
