#include "vendor.h"
#include "core.h"
#include "task_runner.h"
#include "keeper.h"

event_base*  core::base = nullptr;
task_runner* core::task = nullptr;
evdns_base*  core::base_dns = nullptr;
keeper*      core::keep = nullptr;

void core::init(php::extension_entry& extension) {
	extension.add<core::go>("flame\\go");
	extension.add<core::run>("flame\\run");
	extension.add<core::sleep>("flame\\sleep");
	extension.add<core::fork>("flame\\fork");
	extension.on_module_startup([] (php::extension_entry& extension) -> bool {
		// 初始化 core
		evthread_use_pthreads();
		core::base = event_base_new();
		core::task = new task_runner();
		core::base_dns = evdns_base_new(core::base, EVDNS_BASE_INITIALIZE_NAMESERVERS);
		core::keep = new keeper();
		return true;
	});
	extension.on_module_shutdown([] (php::extension_entry& extension) -> bool {
		// 销毁 core
		if(core::base_dns != nullptr) { // 由于 base_dns 会使 core::base 一直存活，可能会被提前销毁
			evdns_base_free(core::base_dns, 1);
		}
		event_base_free(core::base);
		delete core::task;
		delete core::keep;
		libevent_global_shutdown();
		return true;
	});
}

static bool core_forked  = false;
static bool core_started = false;
struct core_event_wrapper {
	php::generator  gn;
	event           ev;
};
// 核心调度逻辑
static bool generator_runner_continue(core_event_wrapper* ew) {
	if(EG(exception) != nullptr) {
		event_base_loopbreak(core::base);
		return false;
	}else if(!ew->gn.valid()) {
		delete ew;
		if(event_base_get_num_events(core::base, EVENT_BASE_COUNT_ADDED) == 2) {
			core::keep->stop();
			evdns_base_free(core::base_dns, 0);
			core::base_dns = nullptr;
		}
		return false;
	}
	return true;
}
// 核心运行逻辑
static void generator_runner_callback(evutil_socket_t fd, short events, void* data) {
	core_event_wrapper* ew = reinterpret_cast<core_event_wrapper*>(data);
	php::value v = ew->gn.current();
	if(v.is_callable()) {
		// 传入的 函数 用于将异步执行结果回馈到 "协程" 中
		static_cast<php::callable&>(v)(php::value([ew] (php::parameters& params) mutable -> php::value {
			int len = params.length();
			if(len == 0) {
				ew->gn.next();
			}else if(params[0].is_empty()) { // 没有错误
				if(len > 1) { // 有回调数据
					ew->gn.send(params[1]);
				}else{
					ew->gn.next();
				}
			}else if(params[0].is_exception()) { // 内置 exception
				ew->gn.throw_exception(params[0]);
			}else{ // 其他错误信息
				ew->gn.throw_exception(params[0], 0);
			}
			if(generator_runner_continue(ew)) {
				event_active(&ew->ev, EV_READ, 0);
			}
			return nullptr;
		}));
	}else{
		ew->gn.send(v);
		if(generator_runner_continue(ew)) {
			event_active(&ew->ev, EV_READ, 0);
		}
	}
}
// 所谓“协程”
php::value core::go(php::parameters& params) {
	if(!core_started) throw php::exception("failed to start coroutine: core not running");
	php::value r;
	if(params[0].is_callable()) {
		r = static_cast<php::callable&>(params[0])();
	}else{
		r = params[0];
	}
	if(r.is_generator()) {
		core_event_wrapper* ew = new core_event_wrapper;
		event_assign(&ew->ev, core::base, -1, EV_READ, generator_runner_callback, ew);
		ew->gn = r;
		event_add(&ew->ev, nullptr);
		event_active(&ew->ev, EV_READ, 0);
		return nullptr;
	}else{
		return std::move(r);
	}
}
// 程序启动
php::value core::run(php::parameters& params) {
	core_started = true;
	core::task->start();
	core::keep->start();
	if(params.length() > 0) {
		go(params);
	}
	event_base_dispatch(core::base);
	if(event_base_got_break(core::base)) {
		core::task->stop();
	}else{
		core::task->wait();
	}
	// !!! 若 keep 还未 stop 程序不会走到这里
	return nullptr;
}

struct timer_wrapper {
	struct timeval to;
	event          ev;
	php::callable  cb;
};
php::value core::sleep(php::parameters& params) {
	int duration = params[0];
	timer_wrapper* tw = new timer_wrapper;
	tw->to = (struct timeval) {
		duration / 1000,
		duration * 1000 % 1000000
	};
	event_assign(&tw->ev, core::base, -1, 0, [] (evutil_socket_t fd, short events, void* data) {
		auto tw = reinterpret_cast<timer_wrapper*>(data);
		tw->cb();
		delete tw;
	}, tw);
	return php::value([tw] (php::parameters& params) -> php::value {
		tw->cb = params[0];
		evtimer_add(&tw->ev, &tw->to);
		return nullptr;
	});
}
php::value core::fork(php::parameters& params) {
#ifndef SO_REUSEPORT
	throw php::exception("failed to fork: SO_REUSEPORT needed");
#endif
	if(core_started) {
		throw php::exception("failed to fork: already running");
	}
	if(core_forked) {
		throw php::exception("failed to fork: already forked");
	}
	core_forked = true;
	int count = params[0];
	if(count <= 0) {
		count = 1;
	}
	for(int i=0;i<count;++i) {
		if(::fork() == 0) break;
	}
	return nullptr;
}
