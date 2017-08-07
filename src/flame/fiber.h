#pragma once

namespace flame {
	// 用于标记异步操作
	extern php::value async;
	// 当前事件循环
	extern uv_loop_t* loop;
	// 包裹一个 generator function 以构建“协程”
	class fiber {
		typedef bool (*STACK_CALLBACK_T)(php::value& v, void* data);
	private:
		php::generator        gen_;
		bool                  run_();
		static fiber*         cur_;
		std::stack<STACK_CALLBACK_T> cbs_;
		std::stack<void*>     ctx_;
		void                  pop_(php::value &rv);
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
	};
	// 注意此函数只能在被 PHP 直接调用的过程中使用，
	// 若在 cb 中可能导致未知行为
	inline fiber* this_fiber(void* data = nullptr) {
		if(!fiber::cur_->ctx_.empty()) {
			throw php::exception("previous async function not finished");
		}
		fiber::cur_->ctx_.push(data);
		fiber::cur_->cbs_.push(nullptr);
		return fiber::cur_;
	}
}

