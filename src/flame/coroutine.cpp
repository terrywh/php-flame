#include "flame.h"
#include "coroutine.h"

namespace flame {
	coroutine* coroutine::current;
	static php::value async_flag;
	void coroutine::prepare() {
		ZVAL_PTR((zval*)&async_flag, nullptr);
	}
	php::value async() {
		return async_flag;
	}
	php::value async(void* context) {
		php::value v;
		ZVAL_PTR((zval*)&v, context);
		return std::move(v);
	}

	void coroutine::run() {
		if(EG(exception) || !generator_.valid()) {
			delete this;
			// 可能由于 valid() 调用，引发新的异常（故需要二次捕获）
			if(EG(exception)) {
				uv_stop(loop);
			}
			return;
		}
		php::value v = generator_.current();
		if(v.is_pointer()) { // 使用内置 IS_PTR 类型标识异步操作
			status_ |= 0x01;
			return;
		}
		generator_.send(std::move(v));
	}

	void coroutine::default_close_cb(uv_handle_t* handle) {
		coroutine_context<uv_handle_t, void>* cc = static_cast<coroutine_context<uv_handle_t, void>*>(handle->data);
		cc->routine()->next();
		delete cc;
	}

	void coroutine::next(php::value&& rv) {
		coroutine* old = current;
		current = this;
		if(yields_.empty()) {
			generator_.send(rv);
			run();
		}else{
			stack_t st = yields_.front();
			yields_.pop_front();
			st.cb(rv, current, st.data);
		}
		current = old;
	}
	void coroutine::next() {
		coroutine* old = current;
		current = this;
		if(yields_.empty()) {
			generator_.next();
			run();
		}else{
			stack_t st = yields_.front();
			yields_.pop_front();
			php::value rv(nullptr);
			st.cb(rv, current, st.data);
		}
		current = old;
	}
	void coroutine::fail(const php::exception& ex) {
		coroutine* old = current;
		current = this;
		while(!yields_.empty()) {
			yields_.pop_front();
		}
		generator_.throw_exception(ex);
		run();
		current = old;
	}

}
