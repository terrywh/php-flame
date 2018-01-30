#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "time.h"
#include "ticker.h"


namespace flame {
namespace time {
	static void sleep_timer_cb(uv_timer_t* handle) {
		coroutine* co = reinterpret_cast<coroutine*>(handle->data);
		co->next();
		uv_close((uv_handle_t*)handle, flame::free_handle_cb);
	}
	static php::value sleep(php::parameters& params) {
		int64_t ms = params[0];
		uv_timer_t* req = (uv_timer_t*)malloc(sizeof(uv_timer_t));
		req->data = coroutine::current;
		uv_timer_init(flame::loop, req);
		uv_timer_start(req, sleep_timer_cb, ms, 0);
		// 标记异步任务的特殊返回值
		return flame::async();
	}
	// 示例：包裹 sleep 异步函数
	// -------------------------------------------------------------------------
	static void sleep2_timer_cb(php::value& rv, coroutine* co, void* data) {
		// std::fprintf(stderr, "sleep2\n");
	}
	static php::value sleep_wrapper(php::parameters& params) {
		sleep(params);
		coroutine::current->async(sleep2_timer_cb);
		return flame::async();
	}
	// -------------------------------------------------------------------------
	static php::value now(php::parameters& params) {
		if(params.length() > 0 && params[0].is_true()) {
			uv_update_time(flame::loop);
		}
		return now();
	}
	static int64_t real_time_diff;
	void init(php::extension_entry& ext) {
		uv_update_time(flame::loop);
		int64_t rtime = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
		real_time_diff = uv_now(flame::loop) - rtime;

		ext.add<sleep>("flame\\time\\sleep");
		ext.add<now>("flame\\time\\now");
		php::class_entry<ticker> class_ticker("flame\\time\\ticker");
		class_ticker.add(php::property_entry("interval", 1000));
		class_ticker.add(php::property_entry("repeat", (zend_bool)true));
		class_ticker.add<&ticker::__construct>("__construct");
		class_ticker.add<&ticker::__destruct>("__destruct");
		class_ticker.add<&ticker::start>("start");
		class_ticker.add<&ticker::stop>("stop");
		ext.add(std::move(class_ticker));
		ext.add<flame::time::tick>("flame\\time\\tick");
		ext.add<flame::time::after>("flame\\time\\after");
	}
	int64_t now() { // 代价较每次取系统时间要低一些
		// uv_update_time(flame::loop);
		return uv_now(flame::loop) - real_time_diff;
	}
	const char* datetime(int64_t st) {
		static char buffer[24];
		st = st/1000;
		struct tm* tt = std::localtime((time_t*)&st);
		sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
			tt->tm_year + 1900,
			tt->tm_mon + 1,
			tt->tm_mday,
			tt->tm_hour,
			tt->tm_min,
			tt->tm_sec);
		
		return buffer;
	}
}
}
