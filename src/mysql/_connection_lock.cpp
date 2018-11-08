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
	_connection_lock::~_connection_lock() {
	}
	// 以下函数应在主线程调用
	_connection_lock& _connection_lock::exec(worker_fn_t&& wk, master_fn_t&& fn) {
		auto ptr = this->shared_from_this();
		// 避免在工作线程中对 wk 捕获的 PHP 对象进行拷贝释放
		auto wk_ = std::make_shared<worker_fn_t>(std::move(wk));
		auto fn_ = std::make_shared<master_fn_t>(std::move(fn));
		boost::asio::post(controller_->context_ex, [this, wk_, fn_, ptr] () {
			int error = 0;
			MYSQL_RES* r = (*wk_)(c_, error);
			boost::asio::post(context, [this, wk_, fn_, r, error, ptr] () {
				(*fn_)(c_, r, error);
			});
		}); // 访问当前已持有的连接
		return *this;
	}
}
}
