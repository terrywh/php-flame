#include "deps.h"
#include "flame.h"
#include "coroutine.h"
#include "coroutine_switch.h"
#include "log/log.h"
#include "log/logger.h"

namespace flame {
	coroutine* coroutine::current;

	void coroutine::prepare() {
		
	}
	php::value async() {
		if(!coroutine::current) {
			php::fail("'async' function cannot be used outside flame coroutine");
		}else if(coroutine::current->status_ != 0) {
			throw php::exception("keyword 'yield' missing before async function");
		}
		++ coroutine::current->status_;
		return php::value(coroutine::current);
	}
	php::value async(php::class_base* cpp) {
		if(!coroutine::current) {
			php::fail("'async' function cannot be used outside flame coroutine");
		}else if(coroutine::current->status_ != 0) {
			throw php::exception("keyword 'yield' missing before async function");
		}
		++ coroutine::current->status_;
		coroutine::current->ref_ = php::object(cpp); // 在协程中保存当前对象的引用（防止异步流程丢失当前对象）
		return php::value(coroutine::current);
	}
	coroutine* coroutine::create(php::callable cb) {
		coroutine* co = new coroutine();
		co->gen_.push(php::value([cb, co] (php::parameters& params) mutable -> php::value {
			coroutine_switch sw(co);
			co->gen_.push(cb.invoke());
			co->run();
			return nullptr;
		}));
		return co;
	}
	coroutine* coroutine::create(php::callable cb, std::vector<php::value> argv) {
		coroutine* co = new coroutine();
		co->gen_.push(php::value([cb, argv, co] (php::parameters& params) mutable -> php::value {
			coroutine_switch sw(co);
			co->gen_.push(cb.invoke(std::move(argv)));
			co->run();
			return nullptr;
		}));
		return co;
	}
	coroutine::coroutine()
	: status_(0)
	, after_(after_t {nullptr, nullptr}) {
		uv_timer_init(flame::loop, &timer_);
		timer_.data = this;
		uv_timer_init(flame::loop, &alive_);
		uv_ref((uv_handle_t*)&alive_);
	}
	coroutine::~coroutine() {
		uv_unref((uv_handle_t*)&alive_);
	}
	void coroutine::close_cb(uv_handle_t* timer) {
		coroutine* self = reinterpret_cast<coroutine*>(timer->data);
		if(php::exception::has()) {
			uv_stop(flame::loop);
		}else if(self->after_.func != nullptr) {
			self->after_.func(self->after_.data);
		}
		delete self;
	}
	void coroutine::resume_cb(uv_timer_t* timer) {
		coroutine* self = reinterpret_cast<coroutine*>(timer->data);
		php::value& g = self->gen_.top(), rv;
		if(php::exception::has()) {
			rv = php::exception::get();
			php::exception::off();
		}else if(g.is_generator()) {
			rv = static_cast<php::generator&>(g).get_return();
		}else{
			rv = g;
		}
		self->gen_.pop();
		self->next(rv);
	}
	void coroutine::close() {
		assert(status_ != -1);
		int i = 0;
		if(status_ > 0) {
			// 缺陷：由于 coroutine 已经在结束过程，无法在原位置进行异常上报
			// 下面错误会提示在 run() 函数位置
			php::fail("keyword 'yield' missing before async function");
		}
		if(gen_.size() == 1) {
			status_ = -1;
			// uv_timer_stop(&timer_);
			uv_close((uv_handle_t*)&timer_, close_cb);
		}else{
			status_ = 0;
			// uv_timer_stop(&timer_);
			uv_timer_start(&timer_, resume_cb, 0, 0);
		}
	}
	void coroutine::run() {
		while(status_ >= 0) {
			php::value& g = gen_.top();
			if(php::exception::has()) {
				status_ = 0; // 已经发生异常，不再处理 status_ 问题
				close();
				break;
			}
			// coroutine 结束，需要确认是否缺少 yield 关键字
			if(!g.is_generator() || !static_cast<php::generator&>(g).valid()) {
				close();
				break;
			}
			php::value y = static_cast<php::generator&>(g).current();
			if(y.is_pointer() && y.ptr<coroutine>() == this) { // 使用内置 IS_PTR 类型标识异步操作
				if(--status_ != 0) {
					// 尽量在执行位置处引发异常以更更准确的提示错误位置
					status_ = 0;
					static_cast<php::generator&>(g).throw_exception("keyword 'yield' missing before async function");
					run();
				}
				break;
			}else if(y.is_generator()) { // 简化 yield from 调用方式可直接 yield
				gen_.push(y);
			}else{
				static_cast<php::generator&>(g).send(std::move(y));
			}
		}
	}
	void coroutine::next(php::value rv) {
		if(status_ < 0) return;
		coroutine_switch sw(this);

		php::generator& g = static_cast<php::generator&>(gen_.top());
		assert(g.is_generator());

		if(rv.is_exception()) {
			while(!stack_.empty()) {
				stack_.pop_front();
			}
			// 清理当前引用
			ref_ = nullptr;
			g.throw_exception(rv);
			run();
		}else if(stack_.empty()) {
			// 清理当前引用
			ref_ = nullptr;
			if(rv.is_null()) g.next(); else g.send(rv);
			run();
		}else{
			stack_t st = stack_.front();
			stack_.pop_front();
			st.func(rv, st.data);
		}
	}
	void coroutine::next() {
		if(status_ < 0) return;
		coroutine_switch sw(this);

		php::generator& g = static_cast<php::generator&>(gen_.top());
		assert(g.is_generator());

		if(stack_.empty()) {
			// 清理当前引用
			ref_ = nullptr;
			g.next();
			run();
		}else{
			stack_t st = stack_.front();
			stack_.pop_front();
			st.func(nullptr, st.data);
		}
	}
	void coroutine::fail(const php::value& e, int code) {
		if(status_ < 0) return;
		coroutine_switch sw(this);

		while(!stack_.empty()) {
			stack_.pop_front();
		}
		php::generator& g = static_cast<php::generator&>(gen_.top());
		assert(g.is_generator());
		g.throw_exception(e, code);
		run();
	}
	void coroutine::start_cb(uv_timer_t* timer) {
		coroutine* co = reinterpret_cast<coroutine*>(timer->data);
		php::callable cb = co->gen_.top();
		co->gen_.pop();
		if(cb.is_callable()) {
			cb.invoke();
		}else{
			assert(0);
		}
	}
	void coroutine::start() {
		// uv_timer_stop(&timer_);
		uv_timer_start(&timer_, start_cb, 0, 0);
	}
}
