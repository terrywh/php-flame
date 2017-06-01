#include "vendor.h"
#include "core.h"
#include "task_runner.h"
#include "keeper.h"

event_base*  core::base = nullptr;
task_runner* core::task = nullptr;
evdns_base*  core::base_dns = nullptr;
keeper*      core::keep = nullptr;
std::size_t core::count = 0;

void core::init(php::extension_entry& extension) {
	extension.add<core::go>("flame\\go");
	extension.add<core::run>("flame\\run");
	extension.add<core::sleep>("flame\\sleep");
	// extension.add<core::fork>("flame\\fork");
	extension.on_module_startup([] (php::extension_entry& extension) -> bool {
		// 初始化 core
		evthread_use_pthreads();
		core::base = event_base_new();
		core::task = new task_runner();
		core::base_dns = evdns_base_new(core::base, EVDNS_BASE_INITIALIZE_NAMESERVERS);
		core::keep = new keeper();
#ifndef SO_REUSEPORT
 		php::warn("SO_REUSEPORT is needed if multiprocess is expected");
#endif
		return true;
	});
	extension.on_module_shutdown([] (php::extension_entry& extension) -> bool {
		// 销毁 core
		evdns_base_free(core::base_dns, 1);
		event_base_free(core::base);
		core::task->stop();
		delete core::task;
		delete core::keep;
		libevent_global_shutdown();
		return true;
	});
}
static void generator_finished(core::generator_wrapper* ew) {
	if(!ew->cb.is_empty()) {
		ew->cb(nullptr);
	}
	delete ew;
}
// 核心调度逻辑
static bool generator_continue(core::generator_wrapper* ew) {
	if(EG(exception) != nullptr) {
		--core::count;
		delete ew;
		event_base_loopbreak(core::base);
		return false;
	}else if(!ew->gn.valid()) {
		if(!ew->cb.is_empty()) {
			ew->cb(nullptr, ew->gn.get_return());
		}
		delete ew;
		--core::count;
		if(core::count == 0) {
			event_base_loopexit(core::base, nullptr);
		}
		return false;
	}else{
		event_active(&ew->ev, EV_READ, 0);
		return true;
	}
}
// 核心运行逻辑
static void generator_callback(evutil_socket_t fd, short events, void* data) {
	core::generator_wrapper* ew = reinterpret_cast<core::generator_wrapper*>(data);
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
			generator_continue(ew);
			return nullptr;
		}));
	}else{
		ew->gn.send(v);
		generator_continue(ew);
	}
}
// 所谓“协程”
core::generator_wrapper* core::generator_start(const php::generator& gn) {
	core::generator_wrapper* ew = new core::generator_wrapper { gn };
	event_assign(&ew->ev, core::base, -1, EV_READ, generator_callback, ew);
	// ew->gn = gn;
	event_add(&ew->ev, nullptr);
	event_active(&ew->ev, EV_READ, 0);
	++core::count;
	return ew;
}
// static bool core_forked  = false;
static bool core_started = false;

php::value core::go(php::parameters& params) {
	if(!core_started) throw php::exception("failed to start coroutine: core not running");
	php::value v = params[0];
	if(v.is_callable()) {
		v = static_cast<php::callable>(v).invoke();
	}
	if(v.is_generator()) {
		core::generator_wrapper* ew = generator_start(v);
		return php::value([ew] (php::parameters& params) -> php::value {
			ew->cb = params[0];
			return nullptr;
		});
	}else{
		if(EG(exception) != nullptr) {
			event_base_loopbreak(core::base);
		}else if(core::count == 0) {
			event_base_loopexit(core::base, nullptr);
		}
		return v;
	}
}
// 程序启动
php::value core::run(php::parameters& params) {
	if(params.length() == 0) {
		throw php::exception("run failed: callable or generator is required");
	}
	core_started = true;
	core::task->start();
	core::keep->start();
	core::go(params);
	if(core::count > 0) {
		event_base_dispatch(core::base);
	}
	core::task->wait();
	core::keep->stop();
	return nullptr;
}

struct core_timer_wrapper {
	struct timeval to;
	event          ev;
	php::callable  cb;
};
php::value core::sleep(php::parameters& params) {
	int duration = params[0];
	core_timer_wrapper* tw = new core_timer_wrapper;
	tw->to = (struct timeval) {
		duration / 1000,
		duration * 1000 % 1000000
	};
	event_assign(&tw->ev, core::base, -1, 0, [] (evutil_socket_t fd, short events, void* data) {
		auto tw = reinterpret_cast<core_timer_wrapper*>(data);
		tw->cb();
		delete tw;
	}, tw);
	return php::value([tw] (php::parameters& params) -> php::value {
		tw->cb = params[0];
		evtimer_add(&tw->ev, &tw->to);
		return nullptr;
	});
}
// php::value core::fork(php::parameters& params) {
// #ifndef SO_REUSEPORT
// 	throw php::exception("failed to fork: SO_REUSEPORT needed");
// #endif
// 	if(core_started) {
// 		throw php::exception("failed to fork: already running");
// 	}
// 	if(core_forked) {
// 		throw php::exception("failed to fork: already forked");
// 	}
// 	core_forked = true;
// 	int count = params[0];
// 	if(count <= 0) {
// 		count = 1;
// 	}
// 	for(int i=0;i<count;++i) {
// 		if(::fork() == 0) break;
// 	}
// 	return nullptr;
// }
