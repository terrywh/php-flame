#pragma once

namespace flame {
namespace os {
	void declare(php::extension_entry& ext);
	php::value interfaces(php::parameters& params);
	php::value spawn(php::parameters& params);
	php::value exec(php::parameters& params);
}
}