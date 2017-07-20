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

		std::stack<void*>      ctx_;
		std::stack<STACK_CB> cbs_;
		void                   pop_(php::value &rv);
	public:
		template <typename ...Args>
		static bool start(php::callable& cb, const Args&... argv) {
			fiber* old = cur_;
			cur_ = new fiber();
			php::value gen = cb(argv...);
			if(gen.is_generator()) {
				cur_->gen_ = std::move(gen);
			}else{
				delete cur_;
				cur_ = old;
				return false;
			}
			return cur_->run_();
		}
		
		inline fiber* push(void* data) {
			ctx_.push(data);
			return this;
		}
		inline fiber* push(const STACK_CB& cb) {
			cbs_.push(cb);
			return this;
		}
		template <typename T>
		inline T* pop() {
			T* t = reinterpret_cast<T*>(ctx_.top());
			ctx_.pop();
			return t;
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
		inline T* top() {
			return reinterpret_cast<T*>(ctx_.top());
		}
		inline void next(php::value rv) {
			pop_(rv);
		}
		inline void next() {
			php::value rv(nullptr);
			pop_(rv);
		}
		friend fiber* this_fiber();
	};
	// 注意次函数只能在被 PHP 直接调用的过程中使用，
	// 若在 cb 中可能导致未知行为
	inline fiber* this_fiber() {
		return fiber::cur_;
	}
}

