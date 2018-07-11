#pragma once

namespace flame {
namespace log {
	class logger;
	extern logger* logger_;
	
	void declare(php::extension_entry& ext);
	php::value rotate(php::parameters& params);
	php::value write(php::parameters& params);
	php::value info(php::parameters& params);
	php::value warn(php::parameters& params);
	php::value fail(php::parameters& params);
}
}