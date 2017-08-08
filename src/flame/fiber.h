#pragma once

namespace flame {
	
	// 当前事件循环
	extern uv_loop_t* loop;
	// 包裹一个 generator function 以构建“协程”
	class fiber {
		typedef bool (*STACK_CALLBACK_T)(php::value& v, void* data);
	private:
		// 用于标记 异步函数在当前“协程”中的执行状态
		// 0 -> 即将开始 -> 1
		// 1 -> 已开始，等待 yield -> 2
		// 2 -> yield 完成 -> 0
		int                   status_;
		php::generator        gen_;
		bool                  run_();
		static fiber*         cur_;
		std::stack<STACK_CALLBACK_T> cbs_;
		std::stack<void*>     ctx_;
		void                  pop_(php::value &rv);
		void                  error_yield_missing_();
		fiber();
	public:
		template <typename ...Args>
		static php::value start(php::callable& cb, const Args&... argv) {
			fiber* old = cur_;
			cur_ = new fiber();
			php::value gen = cb(argv...);
			if(gen.is_generator()) {
				cur_->gen_ = std::move(gen);
			}else{
				delete cur_;
				cur_ = old;
				return gen;
			}
			cur_->run_();
			cur_ = old;
			return nullptr;
		}
		inline fiber* push(STACK_CALLBACK_T cb, void* data) {
			ctx_.push(data);
			cbs_.push(cb);
			return this;
		}
		template <typename T>
		inline T* context() {
			return reinterpret_cast<T*>(ctx_.top());
		}
		inline void next(php::value rv) {
			pop_(rv);
		}
		inline void next() {
			php::value rv(nullptr);
			pop_(rv);
		}
		friend fiber* this_fiber(void* data);
		friend php::value async();
	};
	// 注意此函数只能在被 PHP 直接调用的过程中使用，
	// 若在 cb 中可能导致未知行为
	inline fiber* this_fiber(void* data = nullptr) {
		fiber::cur_->ctx_.push(data);
		fiber::cur_->cbs_.push(nullptr);
		return fiber::cur_;
	}
	// 用于标记异步操作
	extern php::value async_;
	extern inline php::value async() {
		if(fiber::cur_->status_ != 0) {
			fiber::cur_->error_yield_missing_();
			return nullptr;
		}
		++flame::fiber::cur_->status_;
		return flame::async_;
	}
}

