#include "fiber.h"

namespace flame {
	uv_loop_t* loop;
	php::value async;
	fiber* fiber::cur_ = nullptr;
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
			if(v == async) { // 异步流程
				break;
			}
			// 其他数据利率直接返回
			gen_.send(v);
		}
		return true;
	}
	void fiber::pop_(php::value& rv) {
		if(cbs_.empty()) { // 堆栈中的回调都已经结束
			fiber* old = cur_;
			cur_ = this;
			if(rv.is_exception()) {
				gen_.throw_exception(rv);
			}else{
				gen_.send(rv);
			}
			run_();
			cur_ = old;
		}else{ // 还存在上层的“堆栈回调”执行，并等待下次 done 返回
			STACK_CB cb = cbs_.top();
			cbs_.pop();
			cb(rv);
		}
	}
}
