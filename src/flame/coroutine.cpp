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
		if(!coroutine::current ||coroutine::current->status_ != 0) {
			php::fail("keyword 'yield' missing before async function");
			if(coroutine::current) coroutine::current->close();
			uv_stop(flame::loop);
			exit(-1);
			return nullptr;
		}
		++ coroutine::current->status_;
		return php::value(coroutine::current);
	}
	php::value async(php::class_base* cpp) {
		if(!coroutine::current ||coroutine::current->status_ != 0) {
			php::fail("keyword 'yield' missing before async function");
			if(coroutine::current) coroutine::current->close();
			uv_stop(flame::loop);
			exit(-1);
			return nullptr;
		}
		++ coroutine::current->status_;
		coroutine::current->ref_ = cpp; // 在协程中保存当前对象的引用（防止异步流程丢失当前对象）
		return php::value((void*)cpp);
	}
	coroutine::coroutine(coroutine* parent, php::generator&& g)
	: status_(0)
	, parent_(parent)
	, gen_(std::move(g))
	, after_(after_t {nullptr, nullptr}) {
		// 用于防止“协程”未结束时提前结束
		uv_async_init(flame::loop, &async_, nullptr);
		async_.data = this;
	}
	void coroutine::close_cb(uv_handle_t* handle) {
		coroutine* self = reinterpret_cast<coroutine*>(handle->data);
		if(self->parent_ != nullptr) {
			self->parent_->next(self->gen_.get_return());
		}
		if(self->after_.func != nullptr) {
			self->after_.func(self->after_.data);
		}
		delete self;
	}
	void coroutine::close() {
		if(status_ != 0) {
			php::fail("keyword 'yield' missing before async function");
			exit(-1);
		}
		status_ = -1;
		coroutine::current = nullptr;
		uv_close((uv_handle_t*)&async_, close_cb);
	}
	void coroutine::run() {
		while(status_ >= 0) {
			if(EG(exception) || !gen_.valid()) {
				// 可能由于 valid() 调用，引发新的异常（故需要二次捕获）
				if(EG(exception)) {
					uv_stop(flame::loop);
					log::default_logger->panic();
				}else{
					close();
				}
				break;
			}
			php::generator g = gen_.current();
			if(g.is_pointer()) { // 使用内置 IS_PTR 类型标识异步操作
				if(--status_ != 0) {
					php::fail("keyword 'yield' missing before async function");
					status_ = 0;
					close();
					uv_stop(flame::loop);
				}
				break;
			}else if(g.is_generator()) { // 简化 yield from 调用方式可直接 yield
				(new coroutine(this, std::move(g)))->start();
				break;
			}else{
				gen_.send(std::move(g));
			}
		}
	}
	void coroutine::next(php::value& rv) {
		if(status_ < 0) return;
		coroutine* old = current;
		current = this;
		if(rv.is_exception()) {
			while(!stack_.empty()) {
				stack_.pop_front();
			}
			// 清理当前引用
			ref_ = nullptr;
			gen_.throw_exception(rv);
			run();
		}else if(stack_.empty()) {
			// 清理当前引用
			ref_ = nullptr;
			gen_.send(rv);
			run();
		}else{
			stack_t st = stack_.front();
			stack_.pop_front();
			st.func(rv, current, st.data);
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
		gen_.throw_exception(message, code);
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
		gen_.throw_exception(ex);
		run();

		current = old;
	}
}
