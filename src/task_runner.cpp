#include "vendor.h"
#include "core.h"
#include "task_runner.h"

void task_runner::init(php::extension_entry& extension) {
	extension.add<task_runner::async>("flame\\async");
}

task_runner::task_runner()
: base_(event_base_new())
, ev_(event_new(base_, -1, EV_READ, [] (evutil_socket_t fd, short events, void* data) {}, nullptr))
, master_id_(std::this_thread::get_id()) {
	event_add(ev_, nullptr);
}

task_runner::~task_runner() {
	event_free(ev_);
	event_base_free(base_);
}
static int i = 0;
php::value task_runner::async(php::parameters& params) {
	// TODO 禁止从 worker 线程再次 async 异步操作
	task_wrapper* tw = new task_wrapper;
	event_assign(&tw->ev, core::task->base_, -1, EV_READ, async_todo, tw);
	tw->fn = params[0];
	event_add(&tw->ev, nullptr);
	return php::value([tw] (php::parameters& params) mutable -> php::value {
		tw->cb = params[0];
		event_active(&tw->ev, EV_WRITE, 0);
		return nullptr;
	});

}

void task_runner::async_todo(evutil_socket_t fd, short events, void* data) {
	task_wrapper* tw = reinterpret_cast<task_wrapper*>(data);
	event_assign(&tw->ev, core::base, -1, EV_READ, task_runner::async_done, tw);
	event_add(&tw->ev, nullptr);
	tw->fn(php::value([tw] (php::parameters& params) mutable -> php::value {
		if(params.length() > 1) {
			tw->ex = params[0];
			tw->rv = params[1];
		}else if(params.length() > 0) {
			tw->ex = params[0];
		}
		event_active(&tw->ev, EV_WRITE, 0);
		return nullptr;
	}));
}

void task_runner::async_done(evutil_socket_t fd, short events, void* data) {
	task_wrapper* tw = reinterpret_cast<task_wrapper*>(data);
	tw->cb(tw->ex, tw->rv);
	// event_free(tw->ev);
	delete tw;
}
void task_runner::start() {
	std::thread r(run, this);
	worker_ = std::move(r);
}
void task_runner::run(task_runner* self) {
	event_base_dispatch(self->base_);
}
void task_runner::wait() {
	event_active(ev_, EV_READ, 0);
	worker_.join();
}
void task_runner::stop() {
	event_base_loopbreak(base_);
	worker_.join();
}
