#pragma once

namespace net { namespace http {
	class request: public php::class_base {
	public:
		static php::value parse(php::parameters& params);
		php::value __construct(php::parameters& params);
	private:

	};
} }
