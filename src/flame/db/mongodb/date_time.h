#pragma once

namespace flame {
namespace db {
namespace mongodb {
	class date_time: public php::class_base {
	public:
		php::value __construct(php::parameters& params);
		php::value to_string(php::parameters& params);
		php::value jsonSerialize(php::parameters& params);
		inline php::value timestamp(php::parameters& params) {
			return uint32_t(milliseconds_ / 1000);
		}
		inline php::value timestamp_ms(php::parameters& params) {
			return milliseconds_;
		}
		php::value to_datetime(php::parameters& params);
	private:
		int64_t milliseconds_ = 0;
		friend void fill_with(bson_t* doc, const php::array& arr);
		friend php::value from(bson_iter_t* iter);
	};

}
}
}
