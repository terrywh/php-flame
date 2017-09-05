#pragma once

namespace flame {
	extern enum process_type_t {
		PROCESS_MASTER,
		PROCESS_WORKER,
	} process_type;
	// 当前事件循环
	extern uv_loop_t*   loop;
	// 包裹一个 generator function 以构建“协程”
	class fiber {
		// typedef void (*STACK_CALLBACK_T)(php::value& v, void* data);
		typedef std::function<void (php::value& rv)> STACK_CALLBACK_T;
	private:
		// 用于标记异步操作
		static php::value   async_;
		// 用于标记 异步函数在当前“协程”中的执行状态
		// 0 -> 即将开始 -> 0x01
		// 1 -> 已开始，等待 yield -> 0x02
		// 3 ->
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
		static bool start(php::callable& cb, const Args&... argv) {
			fiber* old = cur_;
			cur_ = new fiber();
			php::value gen = cb(argv...);
			if(gen.is_generator()) {
				cur_->gen_ = std::move(gen);
				cur_->run_();
				cur_ = old;
				return true;
			}else{
				cur_->run_();
				cur_ = old;
				return false;
			}
		}
		inline fiber* push() {
			ctx_.push(nullptr);
			return this;
		}
		inline fiber* push(void* data) {
			ctx_.push(data);
			return this;
		}
		inline fiber* push(STACK_CALLBACK_T cb) {
			cbs_.push(cb);
			return this;
		}
		template <typename T>
		inline T* context() {
			if((status_ & 0x03) != 0x03) {
				error_yield_missing_();
				return nullptr;
			}
			return reinterpret_cast<T*>(ctx_.top());
		}
		inline void next(php::value rv) {
			pop_(rv);
		}
		inline void next() {
			php::value rv(nullptr);
			pop_(rv);
		}
		// 同步流程报错，需要清理当前堆栈上下文 状态
		inline void throw_exception(const std::string& msg, int code = -1) {
			while(!ctx_.empty()) ctx_.pop();
			while(!cbs_.empty()) cbs_.pop();
			status_ = 0;
			throw php::exception(msg, code);
		}
		inline void ignore_warning(const std::string& msg, int code = -1) {
			while(!ctx_.empty()) ctx_.pop();
			while(!cbs_.empty()) cbs_.pop();
			status_ = 0;
			php::warn("%s (%d)\n", msg.c_str(), code);
		}
		inline void ignore_notice(const std::string& msg, int code = -1) {
			while(!ctx_.empty()) ctx_.pop();
			while(!cbs_.empty()) cbs_.pop();
			status_ = 0;
			php::info("%s (%d)\n", msg.c_str(), code);
		}
		friend fiber* this_fiber();
		friend php::value async();
		friend void init(php::extension_entry& ext);
	};
	// 注意此函数只能在被 PHP 直接调用的过程中使用，
	// 若在 cb 中可能导致未知行为
	inline fiber* this_fiber() {
		return fiber::cur_;
	}
	php::value async();
}
