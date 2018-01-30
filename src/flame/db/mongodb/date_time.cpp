#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
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
	php::value date_time::to_string(php::parameters& params) {
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
	php::value date_time::to_datetime(php::parameters& params) {
		char sf[24];
		int  ln = 24;
		ln = sprintf(sf, "%ld.%d", milliseconds_ / 1000, (milliseconds_ * 1000) % 1000000);
		php::string format("U.u", 3);
		php::string format_time(sf, ln);
		return php::callable("date_create_from_format").invoke(format, format_time);
	}
}
}
}
