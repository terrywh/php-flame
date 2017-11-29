#pragma once

namespace flame {
namespace time {
	class ticker:public php::class_base {
	public:
		php::value __construct(php::parameters& params);
		php::value start(php::parameters& params);
		php::value stop(php::parameters& params);
		php::value __destruct(php::parameters& params);
		// integer $interval
		// boolean $repeat
	private:
		php::callable cb_;
		// 生命周期需与 PHP 对象不符，故使用动态分配
		uv_timer_t*   tm_;
		// 防止异步过程对象丢失
		php::value   ref_;

		static void tick_cb(uv_timer_t* handle);
	};
	// 简化使用的函数
	php::value after(php::parameters& params);
	php::value tick(php::parameters& params);
}
}
