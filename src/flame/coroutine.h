#pragma once

namespace flame {
	extern uv_loop_t* loop;
	// 包裹一个 generator function 以构建“协程”
	class coroutine {
	private:
		coroutine(coroutine* parent);

		uv_async_t             async_; // 保持引用，同步过程需要用这个 async_ 保持 loop 的活跃
		int                    status_;
		php::generator         generator_;
		coroutine*             parent_;
		typedef void (*callback_t)(php::value& rv, coroutine* co, void* data);
		typedef struct stack_t {
			callback_t cb;
			void*      data;
		} stack_t;
		std::deque<stack_t> yields_; // 用于伪装 yield 指令，实现异步动作的包裹
		template <class K, void (K::*method)(php::value& rv, coroutine* co)>
		static void method_wrapper(php::value& rv, coroutine* co, void* data) {
			(static_cast<K*>(data)->*method)(rv, co);
		}
		void run();
		
		php::object          ref_;
	public:
		static coroutine* current;

		template <typename ...Args>
		static bool start(php::callable& cb, const Args&... argv) {
			php::value gen = cb(argv...);
			if(gen.is_generator()) {
				start(nullptr, std::move(gen));
				return true;
			}else{
				return false;
			}
		}
		static void start(coroutine* parent, php::value&& gen) {
			coroutine* old = current;
			current = new coroutine(parent);
			current->generator_ = std::move(gen);
			current->run();
			current = old;
		}
		void close();
		inline void yield(callback_t cb, void* data = nullptr) {
			yields_.push_back(stack_t {.cb = cb, .data = data});
		}
		template <class K, void (K::*method)(php::value& rv, coroutine* co)>
		void yield(K* k) {
			yields_.push_back(stack_t {.cb = method_wrapper<K, method>, .data = k});
		}
		inline void empty() {
			yields_.clear();
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
		void fail(const php::value& ex);
		inline void fail(const std::string& ex, int code) {
			fail(php::make_exception(ex, code));
		}
		static void prepare();

		friend php::value async();
		friend php::value async(php::class_base* obj);
		friend php::value async(void* context);
	};
	php::value async();
	php::value async(php::class_base* cpp);
}
