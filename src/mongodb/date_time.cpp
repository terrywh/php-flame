#include "date_time.h"
#include "../time/time.h"

namespace flame {
namespace mongodb {
	void date_time::declare(php::extension_entry& ext) {
		php::class_entry<date_time> class_date_time("flame\\mongodb\\date_time");
		class_date_time
			.implements(&php_json_serializable_ce)
			.method<&date_time::__construct>("__construct", {
				{"milliseconds", php::TYPE::INTEGER, false, true}
			})
			.method<&date_time::to_string>("__toString")
			.method<&date_time::to_json>("__toJSON")
			.method<&date_time::to_datetime>("__toDateTime")
			.method<&date_time::unix>("unix")
			.method<&date_time::unix_ms>("unix_ms")
			.method<&date_time::to_json>("jsonSerialize")
			.method<&date_time::to_json>("__debugInfo");
		ext.add(std::move(class_date_time));
	}
	php::value date_time::__construct(php::parameters& params) {
		if(params.size() > 0)
        {
			tm_ = params[0].to_integer();
		}
        else
        {
            // 默认以当前时间建立
			tm_ = std::chrono::duration_cast<std::chrono::milliseconds>(time::now().time_since_epoch()).count();
		}
		return nullptr;
	}
	php::value date_time::to_string(php::parameters& params) {
		return std::to_string(tm_);
	}
	php::value date_time::unix(php::parameters& params) {
		return std::int64_t(tm_ / 1000);
	}
	php::value date_time::unix_ms(php::parameters& params) {
		return tm_;
	}
	php::value date_time::to_datetime(php::parameters& params) {
		return php::datetime(tm_);
	}
	php::value date_time::to_json(php::parameters& params) {
        // 自定义 JSON 形式 (保持和官方 MongoDB 驱动形式一致)
		php::array ret(1), num(1);
		num.set("$numberLong", std::to_string(tm_));
		ret.set("$date", num);
		return std::move(ret);
	}
}
}