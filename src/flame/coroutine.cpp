#include "deps.h"
#include "flame.h"
#include "coroutine.h"
#include "log/log.h"
#include "log/logger.h"

namespace flame {
	coroutine* coroutine::current;

	void coroutine::prepare() {
		
	}
	php::value async() {
		if(!coroutine::current) {
			php::fail("'async' function cannot be used outside flame coroutine");
		}else if(coroutine::current->status_ != 0) {
			throw php::exception("keyword 'yield' missing before async function");
		}
		++ coroutine::current->status_;
		return php::value(coroutine::current);
	}
	php::value async(php::class_base* cpp) {
		if(!coroutine::current) {
			php::fail("'async' function cannot be used outside flame coroutine");
		}else if(coroutine::current->status_ != 0) {
			throw php::exception("keyword 'yield' missing before async function");
		}
		++ coroutine::current->status_;
		coroutine::current->ref_ = php::object(cpp); // 在协程中保存当前对象的引用（防止异步流程丢失当前对象）
		return php::value(coroutine::current);
	}
	coroutine* coroutine::create(php::callable cb) {
		coroutine* co = new coroutine(nullptr);
		co->gen_ = php::value([cb, co] (php::parameters& params) mutable -> php::value {
			coroutine* orig = coroutine::current;
			coroutine::current = co;
			co->gen_ = cb.invoke();
			co->gen_.is_generator() ? co->run() : co->close(); // 非 Generator 型函数调用完成后立即结束
			coroutine::current = orig;
		});
		return co;
	}
	coroutine* coroutine::create(php::callable cb, std::vector<php::value> argv) {
		coroutine* co = new coroutine(nullptr);
		co->gen_ = php::value([cb, argv, co] (php::parameters& params) mutable -> php::value {
			coroutine* orig = coroutine::current;
			coroutine::current = co;
			co->gen_ = cb.invoke(std::move(argv));
			co->gen_.is_generator() ? co->run() : co->close(); // 非 Generator 型函数调用完成后立即结束
			coroutine::current = orig;
		});
		return co;
	}
	coroutine::coroutine(coroutine* parent)
	: status_(0)
	, parent_(parent)
	, after_(after_t {nullptr, nullptr}) {
		uv_timer_init(flame::loop, &timer_);
		timer_.data = this;
		// 用于防止“协程”中的同步过程还未执行时提前结束
		uv_ref((uv_handle_t*)&timer_);
	}
	void coroutine::close_cb(uv_handle_t* handle) {
		coroutine* self = reinterpret_cast<coroutine*>(handle->data);
		// 注意：after_ 回调不能与 parent_ 共同使用
		assert(self->parent_ != nullptr && self->after_ != nullptr);
		if(self->parent_ == nullptr) {
			if(EG(exception)) {
				uv_stop(flame::loop);
				// log::default_logger->panic();
			}else if(self->after_.func != nullptr) {
				self->after_.func(self->after_.data);
			}
		}else{
			if(EG(exception)) {
				// 串联层级调用的异步函数异常
				php::object ex(EG(exception));
				zend_clear_exception();
				self->parent_->fail(ex);
			}else{
				self->parent_->next(static_cast<php::generator&>(self->gen_).get_return());
			}
		}
		delete self;
	}
	void coroutine::close() {
		assert(status_ != -1);
		if(status_ > 0) {
			// 缺陷：由于 coroutine 已经在结束过程，无法在原位置进行异常上报
			// 下面错误会提示在 run() 函数位置
			php::fail("keyword 'yield' missing before async function");
		}
		status_ = -1;
		uv_timer_stop(&timer_);
		uv_unref((uv_handle_t*)&timer_);
		uv_close((uv_handle_t*)&timer_, close_cb);
	}
	void coroutine::run() {
		while(status_ >= 0) {
			if(EG(exception)) {
				status_ = 0; // 已经发生异常，不再处理 status_ 问题
				close();
				break;
			}else if(!static_cast<php::generator&>(gen_).valid()) {
				// coroutine 结束，需要确认是否缺少 yield 关键字
				close();
				break;
			}
			php::generator g = static_cast<php::generator&>(gen_).current();
			if(g.is_pointer() && g.ptr<coroutine>() == this) { // 使用内置 IS_PTR 类型标识异步操作
				if(--status_ != 0) {
					// 尽量在执行位置处引发异常以更更准确的提示错误位置
					status_ = 0;
					static_cast<php::generator&>(gen_).throw_exception("keyword 'yield' missing before async function");
					run();
				}
				break;
			}else if(g.is_generator()) { // 简化 yield from 调用方式可直接 yield
				coroutine* co = new coroutine(this);
				co->gen_ = std::move(g);
				coroutine* old = current;
				current = co;
				co->run();
				current = old;
				break;
			}else{
				static_cast<php::generator&>(gen_).send(std::move(g));
			}
		}
	}
	void coroutine::next(php::value rv) {
		if(status_ < 0) return;
		coroutine* old = current;
		current = this;
		if(rv.is_exception()) {
			while(!stack_.empty()) {
				stack_.pop_front();
			}
			// 清理当前引用
			ref_ = nullptr;
			static_cast<php::generator&>(gen_).throw_exception(rv);
			run();
		}else if(stack_.empty()) {
			// 清理当前引用
			ref_ = nullptr;
			if(rv.is_null()) {
				static_cast<php::generator&>(gen_).next();
			}else{
				static_cast<php::generator&>(gen_).send(rv);
			}
			run();
		}else{
			stack_t st = stack_.front();
			stack_.pop_front();
			st.func(rv, st.data);
		}
		current = old;
	}
	void coroutine::next() {
		if(status_ < 0) return;
		coroutine* old = current;
		current = this;
		if(stack_.empty()) {
			// 清理当前引用
			ref_ = nullptr;
			static_cast<php::generator&>(gen_).next();
			run();
		}else{
			stack_t st = stack_.front();
			stack_.pop_front();
			st.func(nullptr, st.data);
		}
		current = old;
	}
	void coroutine::fail(const std::string& message, int code) {
		if(status_ < 0) return;
		coroutine* old = current;
		current = this;

		while(!stack_.empty()) {
			stack_.pop_front();
		}
		static_cast<php::generator&>(gen_).throw_exception(message, code);
		run();

		current = old;
	}
	void coroutine::fail(php::value ex) {
		if(status_ < 0) return;
		coroutine* old = current;
		current = this;

		while(!stack_.empty()) {
			stack_.pop_front();
		}
		static_cast<php::generator&>(gen_).throw_exception(ex);
		run();

		current = old;
	}
	void coroutine::start_cb(uv_timer_t* handle) {
		coroutine* co = reinterpret_cast<coroutine*>(handle->data);
		if(co->gen_.is_callable()) {
			php::callable cb( std::move(co->gen_) );
			cb.invoke();
		}else{
			assert(0);
		}
	}
	void coroutine::start() {
	   // uv_async_send(&async_);
	   // start_cb(&async_);
	   uv_timer_stop(&timer_);
	   uv_timer_start(&timer_, start_cb, 0, 0);
	}
	// void coroutine::start_cb(uv_async_t* handle) {
	// 	coroutine* co = reinterpret_cast<coroutine*>(handle->data);
	// 	if(co->cb_.is_callable()) {
	// 		static_cast<php::callable&>(co->cb_).invoke();
	// 		// co->cb_ = nullptr;
	// 	}
	// }
}
