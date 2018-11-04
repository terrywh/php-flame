#include "coroutine.h"

namespace flame {
	// 由于 context 销毁时导致剩余的 coroutine 析构, 需要操作 running_
	std::set<coroutine*> coroutine::running_;
	// 必须位于 running_ 之后定义
	boost::asio::io_context    context;
	// 当前协程
	std::shared_ptr<coroutine> coroutine::current;
	php::value coroutine::async() {
		if(current->status_ != 0) {
			throw php::exception(zend_ce_exception, "keyword 'yield' missing (prev)");
		}
		++current->status_;
		return php::value(current.get());
	}
	// 用于切换当前协程
	class switcher {
	public:
		switcher(coroutine* r)
		: o_( std::move(coroutine::current) ) {
			coroutine::current = r->shared_from_this();
		}
		~switcher() {
			o_.swap(coroutine::current);
		}
	private:
		std::shared_ptr<coroutine> o_;
	};
	//
	coroutine::coroutine()
	: status_(0)
	, wk_(context.get_executor()) {
		coroutine::running_.insert(this);
	}
	coroutine::~coroutine() {
		close_ex();
		coroutine::running_.erase(this);
	}
	void coroutine::close_ex() {
		wk_.reset();
		rv_.clear();
		while(!st_.empty()) {
			st_.pop();
		}
	}
	std::shared_ptr<coroutine> coroutine::stack(const php::callable& fn, const php::object& rf) {
		st_.push(std::make_pair(fn, rf));
		return shared_from_this();
	}
	std::shared_ptr<coroutine> coroutine::stack(const php::callable& fn) {
		st_.push(std::make_pair(fn, static_cast<zval*>(nullptr)));
		return shared_from_this();
	}
	// 起动
	void coroutine::start(const php::callable& fn, std::vector<php::value> rv) {
		st_.push(std::make_pair(fn, static_cast<zval*>(nullptr)));
		rv_.swap(rv);
		start_ex();
	}
	// 起动
	void coroutine::start(const php::callable& fn) {
		st_.push(std::make_pair(fn, static_cast<zval*>(nullptr)));
		rv_.clear();
		start_ex();
	}
	// 继续运行
	void coroutine::start_ex() {
		boost::asio::post(context, std::bind(
			static_cast<void (coroutine::*)()>(&coroutine::run), shared_from_this()));
	}
	// 恢复协程并处理可能的异常
	void coroutine::run() {
		if(st_.empty()) {
			if(status_ != 0) {
				throw php::exception(zend_ce_exception, "keyword 'yield' missing (last)");
			}
			return;
		}
		switcher s(this);
		try{
			run_ex();
		}catch(const php::exception& ex) {
			if(!st_.empty()) {
				// 调用过程异常, 抛弃最上层
				st_.pop();
			}
			if (st_.empty()) { // 无堆栈, 抛出 PHP 错误
				// 需要在异常抛出前清理
				throw ex; // -> run()
			} else { // 存在上层堆栈, 将异常作为参数调用
				rv_.push_back(ex);
				start_ex();
				return;
			}
		}
		if(!rv_.empty() && rv_.front().typeof(php::TYPE::POINTER) && rv_.front().pointer<coroutine>() == current.get()) { // 异步标志
			if(--status_ != 0) {
				// 理论上不会出现此种情况
				// 需要在异常抛出前清理
				while(!st_.empty()) st_.pop();
				throw php::exception(zend_ce_exception, "keyword 'yield' missing 3");
			}
			rv_.clear(); // 等待异步流程 resume
		}else{
			// 其他形式的返回值直接带回继续执行
			start_ex();
		}
	}
	//
	void coroutine::resume() {
		rv_.clear();
		start_ex();
	}
	void coroutine::resume(php::value rv) {
		assert(!rv.instanceof(zend_ce_throwable));
		rv_.clear();
		rv_.push_back(rv);
		start_ex();
	}
	void coroutine::resume(std::vector<php::value> rv) {
		rv_.swap(rv);
		start_ex();
	}
	void coroutine::fail(const php::string& msg, int code) {
		rv_.clear();
		if(st_.empty()) {
			throw php::exception(zend_ce_exception, msg, code);
		}else{
			php::exception ex(zend_ce_exception, msg, code);
			if(st_.top().first.instanceof(zend_ce_generator)) {
				tune_ex(st_.top().first, ex); // 修正错位的异常信息(异步流程导致)
			}
			rv_.push_back(ex);
		}
		start_ex();
	}
	void coroutine::fail(const boost::system::error_code& error) {
		fail(error.message(), error.value());
	}
	void coroutine::run_ex() {
		auto top = st_.top();
		std::vector<php::value> rv {std::move(rv_)};
		if (top.first.instanceof(zend_ce_generator)) {
			php::object g = top.first;
			if (rv.empty()) {
				g.call("next");
			} else if (rv[0].instanceof(zend_ce_throwable)) {
				php::exception ex(rv[0]);
				g.call("throw", {ex});
			} else {
				g.call("send", rv);
			}
			if(!g.call("valid")) {
				st_.pop();
				rv_.push_back( g.call("getReturn") );
			}else{
				rv_.push_back( g.call("current") );
			}
		} else if(top.first.typeof(php::TYPE::CALLABLE)) {
			php::callable cb = top.first;
			php::value rs;
			if (rv.empty()) {
				rs = cb.call();
			} else if(rv.front().instanceof(zend_ce_throwable)) { // 函数型回调需要单独处理异常的情况
				throw php::exception(rv.front());
			} else {
				rs = cb.call(rv);
			}
			st_.pop();
			rv_.push_back(rs);
		} else {
			if(rv.front().instanceof(zend_ce_throwable)) {
				throw php::exception(rv.front());
			}
			st_.pop();
			rv_.swap(rv); // 继续传递
		}
		while(!rv_.empty() && rv_.front().instanceof(zend_ce_generator)) {
			php::object g = rv_[0];
			rv_.clear();
			st_.push(std::make_pair(g, static_cast<zval*>(nullptr)));
			php::value rv = g.call("current");
			rv_.push_back( rv );
		}
	}
	void coroutine::tune_ex(const php::object& g, php::exception& ex) {
		zval* exception = ex, rv;
		zend_generator* generator = reinterpret_cast<zend_generator*>(static_cast<zend_object*>(g));
		zend_execute_data *original_execute_data = EG(current_execute_data);
		EG(current_execute_data) = generator->execute_data;
		generator->execute_data->opline--;

		ZVAL_STR(&rv, zend_get_executed_filename_ex());
		zend_update_property(
			zend_get_exception_base(exception), exception, "file", sizeof("file")-1, &rv);

		ZVAL_LONG(&rv, zend_get_executed_lineno());
		zend_update_property(
			zend_get_exception_base(exception), exception, "line", sizeof("line")-1, &rv);

		generator->execute_data->opline++;
		EG(current_execute_data) = original_execute_data;
	}
	void coroutine::shutdown() {
		for(auto i = running_.begin(); i!= running_.end(); ++i) {
			(*i)->close_ex();
		}
	}
}
