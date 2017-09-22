#pragma once

namespace flame {
namespace db {
namespace mongodb {
	class date_time: public php::class_base {
	public:
		php::value __construct(php::parameters& params);
		php::value __toString(php::parameters& params);
		php::value jsonSerialize(php::parameters& params);
		php::value timestamp_ms(php::parameters& params);
		php::value to_datetime(php::parameters& params);
	private:
		int64_t milliseconds_ = 0;
	};

}
}
}
