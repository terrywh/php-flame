#include "vendor.h"
#include "core.h"

boost::asio::io_service* core::io_ = nullptr;

php::value core::error(const boost::system::error_code& err) {
	php::object ex = php::object::create("Exception");
	ex.call("__construct", err.message(), err.value());
	return std::move(ex);
}

void core::init(php::extension_entry& extension) {
	extension.on_module_startup(core::module_startup);
	extension.on_module_shutdown(core::module_shutdown);

	extension.add<core::go>("flame\\go");
	extension.add<core::run>("flame\\run");
	extension.add<core::sleep>("flame\\sleep");
}
bool core::module_startup(php::extension_entry& extension) {
	core::io_ = new boost::asio::io_service();
	return true;
}
bool core::module_shutdown(php::extension_entry& extension) {
	delete core::io_;
	return true;
}
// 核心调度
static void generator_runner(php::object g, bool exception = false) {
	if(!g.scall("valid").is_true()) {
		return;
	}
	core::io().post([g] () mutable {
		php::value v = g.call("current");
		if(v.is_callable()) {
			php::callable cb = v;
			// 传入的 函数 用于将异步执行结果回馈到 "协程" 中
			cb(php::value([g] (php::parameters& params) mutable -> php::value {
				int len = params.length();
				if(len == 0) {
					g.call("next");
					generator_runner(g);
					return nullptr;
				}
				// 首个参数表达错误信息
				if(params[0].is_empty()) { // 没有错误
					if(len > 1) { // 有回调数据
						g.call("send", params[1]);
					}else{
						g.call("next");
					}
					generator_runner(g, false);
				}else if(params[0].is_instance_of("Exception")) { // 内置 exception
					g.call("throw", params[0]);
					generator_runner(g, true);
				}else{ // 其他错误信息
					php::object ex = php::object::create("Exception");
					ex.call("__construct", params[0].to_string());
					g.call("throw", std::move(ex));
					generator_runner(g, true);
				}
				return nullptr;
			}));
		}else{
			g.call("send", v);
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
		generator_runner(r);
		return nullptr;
	}else{
		return std::move(r);
	}
	return nullptr;
}
// 程序启动
php::value core::run(php::parameters& params) {
	if(params.length() > 0) {
		go(params);
	}
	core::io().run();
	return nullptr;
}
// 异步流程 1
php::value core::sleep(php::parameters& params) {
	int duration = params[0];

	return php::value([duration] (php::parameters& params) -> php::value {
		auto timer = std::make_shared<boost::asio::deadline_timer>(core::io());
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
