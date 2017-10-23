#include "flame.h"
#include "coroutine.h"

namespace flame {
	coroutine* coroutine::current;
	static php::value async_flag;
	void coroutine::prepare() {
		ZVAL_PTR((zval*)&async_flag, nullptr);
	}
	php::value async() {
		++ coroutine::current->status_;
		return async_flag;
	}
	php::value async(void* context) {
		++ coroutine::current->status_;
		php::value v;
		ZVAL_PTR((zval*)&v, context);
		return std::move(v);
	}
	coroutine::coroutine(coroutine* parent)
	: status_(0)
	, parent_(parent) {
		// 用于防止“协程”未结束时提前结束
		uv_async_init(flame::loop, &handler_, nullptr);
	}
	coroutine::~coroutine() {
		uv_close((uv_handle_t*)&handler_, nullptr);
	}
	void coroutine::run() {
		while(true) {
			if(EG(exception) || !generator_.valid()) {
				// 可能由于 valid() 调用，引发新的异常（故需要二次捕获）
				if(EG(exception)) {
					uv_stop(flame::loop);
				}else if(this->parent_ != nullptr) {
					coroutine* parent = this->parent_;
					php::value rv = generator_.get_return();
					parent->next(rv);
				}
				delete this;
				break;
			}
			php::value v = generator_.current();
			if(v.is_generator()) { // 简化 yield from 调用方式可直接 yield
				coroutine::start(this, std::move(v));
				break;
			}else if(v.is_pointer()) { // 使用内置 IS_PTR 类型标识异步操作
				if(--status_ != 0) {
					php::fail("keyword 'yield' missing before async function");
					uv_stop(flame::loop);
				}
				break;
			}else{
				generator_.send(std::move(v));
			}
		}
	}
	void coroutine::default_close_cb(uv_handle_t* handle) {
		coroutine_context<uv_handle_t, void>* cc = static_cast<coroutine_context<uv_handle_t, void>*>(handle->data);
		cc->routine()->next();
		delete cc;
	}
	void coroutine::next(php::value& rv) {
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
