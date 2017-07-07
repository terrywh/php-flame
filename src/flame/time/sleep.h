#include "../promise.h"

namespace flame {
namespace time {
	class sleep_fn : public flame::promise {
	public:
		sleep_fn();
		static php::value sleep(php::parameters& params);
	private:
		uv_timer_t  timer_;
		static void timer_cb(uv_timer_t* timer);
	};

	php::value sleep(php::parameters& params);
}
}
