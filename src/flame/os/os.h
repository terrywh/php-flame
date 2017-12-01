#pragma once

namespace flame {
namespace os {
	php::value exec(php::parameters& params);
	void init(php::extension_entry& ext);
}
}
