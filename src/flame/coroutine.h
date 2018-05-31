#pragma once

namespace flame {
	extern uv_loop_t* loop;
	// 包裹一个 generator function 以构建“协程”
	class coroutine {
	private:
		coroutine();
		int                    status_;
		std::stack<php::value>    gen_;
		// 用于异步启动\结束协程
		uv_timer_t              timer_;
		
		typedef void (*async_cb_t)(php::value rv, void* data);
		typedef struct stack_t {
			async_cb_t func;
			void*      data;
		} stack_t;
		std::deque<stack_t> stack_; // 用于伪装 yield 指令，实现异步动作的包裹
		typedef void (*after_cb_t)(void* data);
		typedef struct after_t {
			after_cb_t func;
			void*      data;
		} after_t;
		after_t          after_; // 用于串联
		void run();
		
		php::object          ref_;
		// static void start_cb(uv_async_t* async);
		static void start_cb(uv_timer_t* timer);
		static void resume_cb(uv_timer_t* timer);
		static void close_cb(uv_handle_t* handle);
	public:
		static coroutine* current;
		static coroutine* create(php::callable cb);
		static coroutine* create(php::callable cb, std::vector<php::value> argv);
		void start();
		void close();
		inline void async(async_cb_t cb, void* data = nullptr) {
			stack_.push_back(stack_t {cb, data});
		}
		inline coroutine* after(after_cb_t cb, void* data = nullptr) {
			after_.func = cb;
			after_.data = data;
			return this;
		}
		inline void empty() {
			stack_.clear();
		}
		void next(php::value rv);
		void next();
		void fail(const std::string& message, int code = 0);
		void fail(php::value ex);
		static void prepare();

		friend php::value async();
		friend php::value async(php::class_base* obj);
	};
	php::value async();
	php::value async(php::class_base* cpp);
}
