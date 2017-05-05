#include "vendor.h"
#include "core.h"
//#include "zend_generators.h"

boost::asio::io_service* core::io_ = nullptr;

php::value core::error_to_exception(const boost::system::error_code& err) {
	php::object ex = php::object::create("Exception");
	ex.call("__construct", err.message(), err.value());
	return std::move(ex);
}
// 为了提高 sleep 函数的效率，考虑复用少量 timer
static std::vector<boost::asio::deadline_timer*> timers;

void core::init(php::extension_entry& extension) {
	extension.on_module_startup(core::module_startup);
	extension.on_module_shutdown(core::module_shutdown);

	extension.add<core::go>("flame\\go");
	extension.add<core::run>("flame\\run");
	extension.add<core::sleep>("flame\\sleep");
	extension.add<core::_fork>("flame\\fork");
}
bool core::module_startup(php::extension_entry& extension) {
	core::io_ = new boost::asio::io_service();
	for(int i=0;i<16;++i) {
		timers.push_back(new boost::asio::deadline_timer(core::io()));
	}
	return true;
}
bool core::module_shutdown(php::extension_entry& extension) {
	while(!timers.empty()) {
		delete timers.back();
		timers.pop_back();
	}
	delete core::io_;
	return true;
}
// 核心调度
static void generator_runner(php::generator g) {
	if(EG(exception)) {
		core::io().stop();
		return;
	}else if(!g.valid()) {
		return;
	}
	core::io().post([g] () mutable {
		php::value v = g.current();
		if(v.is_callable()) {
			php::callable cb = v;
			// 传入的 函数 用于将异步执行结果回馈到 "协程" 中
			cb(php::value([g] (php::parameters& params) mutable -> php::value {
				int len = params.length();
				if(len == 0) {
					g.next();
				}else if(params[0].is_empty()) { // 没有错误
					if(len > 1) { // 有回调数据
						g.send(params[1]);
					}else{
						g.next();
					}
				}else if(params[0].is_instance_of("Exception")) { // 内置 exception
					g.throw_exception(params[0]);
				}else{ // 其他错误信息
					g.throw_exception(params[0], 0);
				}
				generator_runner(g);
				return nullptr;
			}));
		}else{
			g.send(v);
			generator_runner(g);
		}
	});
}
// 所谓“协程”
php::value core::go(php::parameters& params) {
	php::value r;
	if(params[0].is_callable()) {
		php::callable c = params[0];
		r = c();
	}else{
		r = params[0];
	}
	if(r.is_object() && r.is_instance_of("Generator")) {
		generator_runner(static_cast<zend_object*>(r));
		return nullptr;
	}else{
		return std::move(r);
	}
	return nullptr;
}
static bool started = false;
// 程序启动
php::value core::run(php::parameters& params) {
	started = true;
	if(params.length() > 0) {
		go(params);
	}
	core::io().run();
	return nullptr;
}
// sleep 对 timer 进行预分配的重用优化，实际上这个流程可能不会有太实际的优化效果，
// 这里的优化更多的是对后面其他对象、函数实现的一种示范
php::value core::sleep(php::parameters& params) {
	int duration = params[0];

	return php::value([duration] (php::parameters& params) -> php::value {
		std::shared_ptr<boost::asio::deadline_timer> timer;

		if(timers.empty()) { // 没有空闲的预分配 timer 需要创建新的
			timer.reset(new boost::asio::deadline_timer(core::io()));
		}else{ // 重复使用这些 timer
			timer.reset(timers.back(), [] (boost::asio::deadline_timer *timer) {
				timers.push_back(timer); // 用完放回去
			});
			timers.pop_back();
		}

		timer->expires_from_now(boost::posix_time::milliseconds(duration));
		php::callable done = params[0];
		timer->async_wait([timer, done] (const boost::system::error_code& ec) mutable {
			if(ec) {
				done(ec.message());
			}else{
				done(nullptr);
			}
		});
		return nullptr;
	});
}

static bool forked = false;
php::value core::_fork(php::parameters& params) {
#ifndef SO_REUSEPORT
	throw php::exception("failed to fork: SO_REUSEPORT needed");
#endif
	if(started) {
		throw php::exception("failed to fork: already running");
	}
	if(forked) {
		throw php::exception("failed to fork: already forked");
	}
	forked = true;
	int count = params[0];
	if(count <= 0) {
		count = 1;
	}
	for(int i=0;i<count;++i) {
		if(fork() == 0) break;
	}
	return nullptr;
}
