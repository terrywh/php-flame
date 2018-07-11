#include "../controller.h"
#include "../coroutine.h"
#include "_connection_base.h"
#include "_connection_lock.h"

namespace flame {
namespace mysql {
	_connection_lock::_connection_lock(std::shared_ptr<MYSQL> c)
	: c_(c) {
		i_ = c->charset;
		s_ = c->server_status;
	}
	// 以下函数应在主线程调用
	_connection_lock& _connection_lock::exec(std::function<std::shared_ptr<MYSQL_RES> (std::shared_ptr<MYSQL> c, int& error)> wk,
			std::function<void (std::shared_ptr<MYSQL> c, std::shared_ptr<MYSQL_RES> r, int error)> fn) {
		auto ptr = this->shared_from_this();
		boost::asio::post(controller_->context_ex, [this, wk, fn, ptr] () {
			int error = 0;
			std::shared_ptr<MYSQL_RES> r = wk(c_, error);
			boost::asio::post(context, [this, fn, r, error, ptr] () {
				fn(c_, r, error);
			});
		}); // 访问当前已持有的连接
		return *this;
	}
}
}