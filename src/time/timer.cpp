#include "../coroutine.h"
#include "timer.h"

namespace flame {
namespace time {
	void timer::declare(php::extension_entry& ext) {
		php::class_entry<timer> class_timer("flame\\time\\timer");
		class_timer
			.property({"interval", 0})
			.property({"stopped",  false})
			.method<&timer::__construct>("__construct", {
				{"interval", php::TYPE::INTEGER},
			})
			.method<&timer::start>("start", {
				{"handler", php::TYPE::CALLABLE},
				{"once", php::TYPE::BOOLEAN, false, true}
			})
			.method<&timer::close>("close");
		ext.add(std::move(class_timer));
	}
	timer::timer()
	: tm_(context)
	, cb_(static_cast<zval*>(nullptr)) {

	}
	php::value timer::__construct(php::parameters& params) {
		// 参数类型已限定 ( 此形式绕过类型检查 )
		std::int64_t d = params[0].to_integer();
		if(d <= 0) {
			throw php::exception(zend_ce_error, "timer interval must be >= 0 milliseconds");
		}
		set("interval", d);
		
		return nullptr;
	}
	php::value timer::start(php::parameters& params) {
		co_ = std::make_shared<coroutine>();
		cb_ = params[0];
		if(params.size() > 1) {
			start_ex(params[1]);
		}else{
			start_ex();
		}
		return php::value(this);
	}
	void timer::start_ex(bool once) {
		std::int64_t duration = get("interval");
		tm_.expires_after(std::chrono::milliseconds(duration));
		set("stopped", once);
		php::object ref(this); // 异步引用, 防止当前对象丢失
		tm_.async_wait([this, ref] (const boost::system::error_code& error) {
			if(!error) {
				// 启动协程执行回调
				co_
				/* 包裹下层回调 */ ->stack(php::callable([this] (php::parameters& params) -> php::value {
					if(!get("stopped")) { // 用户未停止则继续 TICK
						start_ex();
					}else{
						co_.reset();
						cb_ = nullptr;
					}
					return nullptr;
				}), ref)
				/* 启动协程, 回调 */->start(cb_, {ref});
			}
		});
	}
	php::value timer::close(php::parameters& params) {
		set("stopped", true);
		tm_.cancel();
		return nullptr;
	}
}
}