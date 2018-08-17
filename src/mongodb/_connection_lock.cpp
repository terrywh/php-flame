#include "../controller.h"
#include "../coroutine.h"
#include "_connection_base.h"
#include "_connection_lock.h"

namespace flame {
namespace mongodb {
	_connection_lock::_connection_lock(std::shared_ptr<mongoc_client_t> c)
	: c_(c) {

	}
	_connection_lock& _connection_lock::exec(worker_fn_t&& wk, master_fn_t&& fn) {
		auto ptr = this->shared_from_this();
		// 避免在工作线程中对 wk 捕获的 PHP 对象进行拷贝释放
		auto wk_ = std::make_shared<worker_fn_t>(std::move(wk));
		auto fn_ = std::make_shared<master_fn_t>(std::move(fn));
		// 工作线程
		boost::asio::post(controller_->context_ex, [this, wk_, fn_, ptr] () {
			std::shared_ptr<bson_error_t> error;
			// 执行命令
			std::shared_ptr<bson_t> r = (*wk_)(c_, error);
			// 后续流程 (回到主线程)
			boost::asio::post(context, [this, fn_, wk_, r, error, ptr] () {
				(*fn_)(c_, r, error);
			});
		});
		return *this;
	}
}
}
