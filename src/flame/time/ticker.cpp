#include "ticker.h"
#include "../coroutine.h"

namespace flame {
namespace time {
	php::value ticker::__construct(php::parameters& params) {
		prop("interval", 8) = static_cast<int>(params[0]);
		if(params.length() > 1) {
			prop("repeat", 6) = params[1].is_true();
		}else{
			prop("repeat", 6) = bool(true);
		}
		uv_timer_init(flame::loop, &tm_);
		tm_.data = this;
	}
	void ticker::tick_cb(uv_timer_t* handle) {
		ticker* self = static_cast<ticker*>(handle->data);
		self->cb_.invoke(self);
		if(!self->prop("repeat").is_true()) {
			// 非重复定时器立即清理引用
			self->refer_ = nullptr;
		}
	}
	php::value ticker::start(php::parameters& params) {
		cb_ = params[0];

		int iv = prop("interval", 8);
		if(prop("repeat", 6).is_true()) {
			uv_timer_start(&tm_, tick_cb, iv, iv);
		}else{
			uv_timer_start(&tm_, tick_cb, iv, 0);
		}
		// 异步引用
		refer_ = this;
		return nullptr;
	}
	php::value ticker::stop(php::parameters& params) {
		uv_timer_stop(&tm_);
		refer_ = nullptr;
		return nullptr;
	}

	php::value after(php::parameters& params) {
		php::object tick = php::object::create<ticker>();
		tick.call("__construct", params[0], bool(false));
		php::callable cb = params[1];
		tick.call("start", cb);
		return tick;
	}
	php::value tick(php::parameters& params) {
		php::object tick = php::object::create<ticker>();
		tick.call("__construct", params[0], bool(true));
		php::callable cb = params[1];
		tick.call("start", cb);
		return tick;
	}
}
}
