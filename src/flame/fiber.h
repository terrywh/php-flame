#pragma once

namespace flame {
	// 用于标记异步操作
	extern php::value async;
	// 当前事件循环
	extern uv_loop_t* loop;
	// 包裹一个 generator function 以构建“协程”
	class fiber {
		typedef std::function<bool (php::value& v)> STACK_CB;
	private:
		php::generator        gen_;
		bool                  run_();
		static fiber*         cur_;
		void*                 ctx_;
		std::stack<STACK_CB>  cbs_;
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
		inline fiber* push(const STACK_CB& cb) {
			cbs_.push(cb);
			return this;
		}
		inline STACK_CB pop() {
			STACK_CB cb = cbs_.top();
			cbs_.pop();
			return std::move(cb);
		}
		inline STACK_CB top() {
			STACK_CB cb = cbs_.top();
			return std::move(cb);
		}
		template <typename T>
		inline T* context() {
			// return reinterpret_cast<T*>(ctx_.top());
			return reinterpret_cast<T*>(ctx_);
		}
		inline void next(php::value rv) {
			ctx_ = flame::loop;
			pop_(rv);
		}
		inline void next() {
			ctx_ = flame::loop;
			php::value rv(nullptr);
			pop_(rv);
		}
		friend fiber* this_fiber(void* data);
	};
	// 注意此函数只能在被 PHP 直接调用的过程中使用，
	// 若在 cb 中可能导致未知行为
	inline fiber* this_fiber(void* data = nullptr) {
		if(fiber::cur_->ctx_ != flame::loop) {
			throw php::exception("previous async function not finished");
		}
		fiber::cur_->ctx_ = data;
		return fiber::cur_;
	}
}

