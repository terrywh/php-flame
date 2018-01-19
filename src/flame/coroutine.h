#pragma once

namespace flame {
	extern uv_loop_t* loop;
	// 包裹一个 generator function 以构建“协程”
	class coroutine {
	private:
		coroutine(coroutine* parent, php::generator&& g);
		int                    status_;
		coroutine*             parent_;
		php::generator         gen_;
		uv_async_t             async_; // 保持引用，同步过程需要用这个 async_ 保持 loop 的活跃
		
		typedef void (*async_cb_t)(php::value& rv, coroutine* co, void* data);
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
	public:
		static coroutine* current;

		template <typename ...Args>
		static coroutine* create(php::callable& cb, const Args&... argv) {
			php::generator gen = cb(argv...);
			if(gen.is_generator()) {
				return new coroutine(nullptr, std::move(gen));
			}else{
				return nullptr;
			}
		}
		template <typename ...Args>
		static void start(php::callable& cb, const Args&... argv) {
			php::generator gen = cb(argv...);
			if(gen.is_generator()) {
				(new coroutine(nullptr, std::move(gen)))->start();
			}
		}
		void start() {
			coroutine* old = current;
			current = this;
			this->run();
			current = old;
		}
		static void close_cb(uv_handle_t* handle);
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
		void next(php::value& rv);
		inline void next(php::value&& rv) {
			php::value val(std::move(rv));
			next(val);
		}
		void next() {
			php::value val(nullptr);
			next(val);
		}
		void fail(const std::string& message, int code = 0);
		void fail(php::value ex);
		static void prepare();

		friend php::value async();
		friend php::value async(php::class_base* obj);
		friend php::value async(void* context);
	};
	php::value async();
	php::value async(php::class_base* cpp);
}
