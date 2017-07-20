#include "sleep.h"
#include "../fiber.h"

namespace flame {
namespace time {
	static void timer_cb(uv_timer_t* tm) {
		flame::fiber* f = (fiber*)tm->data;
		f->next(); // 不需要带回具体的值
	}
	php::value sleep(php::parameters& params) {
		uv_timer_t* tm = new uv_timer_t;
		// 记录当前协程指针，并在回调函数中恢复
		tm->data = flame::this_fiber();
		int64_t ms = params[0];
		uv_timer_init(flame::loop, tm);
		uv_timer_start(tm, timer_cb, ms, 0);
		// 标记异步任务的特殊返回值
		return flame::async;
	}
}
}
