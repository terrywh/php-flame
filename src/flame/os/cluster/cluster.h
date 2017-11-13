#pragma once

namespace flame {
namespace os {
namespace cluster {
	class messenger;
	
	extern messenger* default_msg;
	void init(php::extension_entry& ext);
}
}
}
