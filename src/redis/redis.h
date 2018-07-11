#pragma once

namespace flame {
namespace redis {
	void declare(php::extension_entry& ext);
	php::value connect(php::parameters& params);
}
}