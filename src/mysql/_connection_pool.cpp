#include "../controller.h"
#include "../coroutine.h"
#include "_connection_base.h"
#include "_connection_pool.h"

namespace flame {
namespace mysql {
	_connection_pool::_connection_pool(std::shared_ptr<php::url> url, std::size_t max)
	: url_(url)
	, max_(max)
	, size_(0)
	, wait_guard(controller_->context_ex) {

	}
	_connection_pool::~_connection_pool() {
		while(!conn_.empty()) {
			mysql_close(conn_.front());
			conn_.pop_front();
		}
	}
	// 以下函数应在主线程调用
	_connection_pool& _connection_pool::exec(worker_fn_t&& wk, master_fn_t&& fn) {
		auto ptr = this->shared_from_this();
		// 避免在工作线程中对 wk 捕获的 PHP 对象进行拷贝释放
		auto wk_ = std::make_shared<worker_fn_t>(std::move(wk));
		auto fn_ = std::make_shared<master_fn_t>(std::move(fn));
		// 受保护的连接获取过程
		boost::asio::post(wait_guard, std::bind(&_connection_pool::acquire, this, [wk_, fn_, ptr] (std::shared_ptr<MYSQL> c) {
			// 不受保护的工作过程
			boost::asio::post(controller_->context_ex, [wk_, fn_, c, ptr] () {
				int error = 0;
				MYSQL_RES* r = (*wk_)(c, error);
				boost::asio::post(context, [wk_, fn_, c, r, error, ptr] () {
					// 主线程后续流程
					(*fn_)(c, r, error);
				});
			});
		}));
		return *this;
	}
	// 以下函数应在工作线程调用
	void _connection_pool::acquire(std::function<void (std::shared_ptr<MYSQL> c)> cb) {
		wait_.push_back(std::move(cb));
		while(!conn_.empty()) {
			if(mysql_ping(conn_.front()) == 0) { // 找到了一个可用连接
				release(conn_.front());
				conn_.pop_front();
				return;
			}else{ // 已丢失的连接抛弃
				--size_;
				conn_.pop_front();
			}
		}
		if(size_ >= max_) return; // 已建立了足够多的连接, 需要等待

		MYSQL* c = mysql_init(nullptr);
		if(!mysql_real_connect(c, url_->host, url_->user, url_->pass, url_->path + 1, url_->port, nullptr, 0)) {
			throw std::runtime_error("cannot connect to mysql server");
		}
		// 提供给 escape 使用(主线程)
		i_ = c->charset;
		s_ = c->server_status;
		++size_; // 当前还存在的连接数量
		release(c);
	}
	void _connection_pool::release(MYSQL* c) {
		if(wait_.empty()) { // 无等待分配的请求
			conn_.push_back(c);
		}else{ // 立刻分配使用
			std::function<void (std::shared_ptr<MYSQL> c)> cb = wait_.front();
			wait_.pop_front();
			auto ptr = this->shared_from_this();
			std::shared_ptr<MYSQL> p(c, [this, ptr] (MYSQL* c) {
				boost::asio::post(wait_guard, std::bind(&_connection_pool::release, ptr, c));
			});
			cb(p);
		}
	}
}
}
