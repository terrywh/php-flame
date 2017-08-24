#include "fiber.h"
#include "flame.h"
#include "process_manager.h"

namespace flame {
	// 异步函数标记
	php::value fiber::async_;
	// 当前“协程”
	fiber* fiber::cur_ = nullptr;
	fiber::fiber():status_(0) {
		// 使用 flame::loop 作为上下文的特殊标记
	}
	// 协程的执行过程
	bool fiber::run_() {
		if(gen_.is_empty()) { // 非 generator 直接继续
			if(status_ == 0) { // 无异步操作
				delete this;
			}else if(status_ & 0x02) { // 内部异步 yield 操作标志
				error_yield_missing_();
				return false;
			}else{
				status_ |= 0x02;
			}
		}else{ // 基于 generator 的操作
SYNC_NEXT:
			if(EG(exception) || !gen_.valid()) {
				// 协程结束，进行回收
				delete this;
				if(EG(exception)) {
				// 上面的 valid 调用过程可能引起新的 exception 生成
				// (若生成的 exception 被 try/catch 这里不会捕获到)
					uv_stop(flame::loop);
				}
				return false;
			}
			php::value v = gen_.current();
			if(v == async_) { // 异步流程 yield 标志
				status_ |= 0x02;
				goto ASYNC_NEXT;
			}
			// 其他数据利率直接返回
			gen_.send(v);
			goto SYNC_NEXT;
		}
ASYNC_NEXT:
		return true;
	}
	void fiber::error_yield_missing_() {
		php::fail("missing keyword 'yield' for async function");
		uv_stop(flame::loop);
		exit(-2);
	}
	void fiber::pop_(php::value& rv) {
		if((status_ & 0x03) != 0x03) { // 未开始异步过程 或 缺少 yield 关键字
			error_yield_missing_();
			return;
		}
		ctx_.pop();
		if(cbs_.empty()) { // 堆栈中的回调都已经结束
			fiber* old = cur_;
			cur_ = this;
			status_ = 0; // 恢复即将开始异步函数的状态
			if(!gen_.is_empty()) {
				if(rv.is_exception()) {
					gen_.throw_exception(rv);
				}else{
					gen_.send(rv);
				}
			}
			run_();
			cur_ = old;
		}else{ // 还存在上层的“堆栈回调”执行，并等待下次 done 返回
			auto cb = cbs_.top();
			cbs_.pop();
			cb(rv);
		}
	}
	php::value async() {
		if((fiber::cur_->status_ & 0x01) == 0x01) {
			fiber::cur_->error_yield_missing_();
			return nullptr;
		}
		fiber::cur_->status_ |= 0x01;
		return fiber::async_;
	}
}
