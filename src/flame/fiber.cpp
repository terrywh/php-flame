#include "fiber.h"

namespace flame {
	uv_loop_t* loop;
	// 异步函数标记
	php::value async_;
	// 当前“协程”
	fiber* fiber::cur_ = nullptr;
	fiber::fiber():status_(0) { 
		// 使用 flame::loop 作为上下文的特殊标记
	}
	// 协程的执行过程
	bool fiber::run_() {
		while(true) { // TODO 是否需要所有协程的统一的退出标记？
			if(EG(exception) || !gen_.valid()) {
				// 协程结束，进行回收
				if(cur_ == this) cur_ = nullptr;
				delete this;
				if(EG(exception)) {
				// 上面的 valid 调用过程可能引起新的 exception 生成
				// (若生成的 exception 被 try/catch 这里不会捕获到)
					uv_stop(flame::loop);
				}
				return false;
			}
			php::value v = gen_.current();
			if(v == async_) { // 异步流程
				++status_;
				break;
			}
			// 其他数据利率直接返回
			gen_.send(v);
		}
		return true;
	}
	void fiber::error_yield_missing_() {
		php::fail("missing keyword 'yield' for async function");
		uv_stop(flame::loop);
	}
	void fiber::pop_(php::value& rv) {
		if(status_ != 2) { // 未开始异步过程 或 缺少 yield 关键字
			error_yield_missing_();
			return;
		}
		cbs_.pop();
		ctx_.pop();
		if(cbs_.empty()) { // 堆栈中的回调都已经结束
			fiber* old = cur_;
			cur_ = this;
			status_ = 0; // 恢复即将开始异步函数的状态
			if(rv.is_exception()) {
				gen_.throw_exception(rv);
			}else{
				gen_.send(rv);
			}
			run_();
			cur_ = old;
		}else{ // 还存在上层的“堆栈回调”执行，并等待下次 done 返回
			void* data = ctx_.top();
			auto  cb   = cbs_.top();
			ctx_.pop();
			cbs_.pop();
			cb(rv, data);
		}
	}
}
