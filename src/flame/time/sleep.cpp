#include "../promise.h"
#include "sleep.h"

namespace flame {
namespace time {
	sleep_fn::sleep_fn() {
		uv_timer_init(uv_default_loop(), &timer_);
		timer_.data = this;
	}
	php::value sleep_fn::sleep(php::parameters& params) {
		sleep_fn* sf = new sleep_fn();
		int64_t ms = params[0];
		uv_timer_start(&sf->timer_, timer_cb, ms, 0);
		return sf->future_;
	}
	void sleep_fn::timer_cb(uv_timer_t* timer_) {
		sleep_fn* sf = reinterpret_cast<sleep_fn*>(timer_->data);
		sf->resolve();
		delete sf;
	}

}
}
