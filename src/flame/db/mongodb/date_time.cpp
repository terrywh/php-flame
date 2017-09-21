#include "../../fiber.h"
#include "date_time.h"

namespace flame {
namespace db {
namespace mongodb {
	php::value date_time::__construct(php::parameters& params) {
		if(params.length() >= 1 && params[0].is_long()) {
			milliseconds_ = params[0];
		}else{
			struct timeval tv;
			bson_gettimeofday(&tv);
			milliseconds_ = tv.tv_sec * 1000 + tv.tv_usec/1000;
		}
		return nullptr;
	}
	php::value date_time::__toString(php::parameters& params) {
		char str[24];
		int  len = 24;
		len = snprintf(str, len, "%ld", milliseconds_);
		return php::string(str, len);
	}
	php::value date_time::jsonSerialize(php::parameters& params) {
		php::array date_(1), long_(1);
		char str[24];
		int  len = 24;
		len = snprintf(str, len, "%ld", milliseconds_);
		long_["$numberLong"] = php::string(str, len);
		date_["$date"] = std::move(long_);

		return std::move(date_);
	}
	php::value date_time::timestamp(php::parameters& params) {
		return uint32_t(milliseconds_/1000);
	}
}
}
}
