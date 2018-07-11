#include "../controller.h"
#include "../coroutine.h"
#include "_connection_base.h"
#include "_connection_lock.h"

namespace flame {
namespace mongodb {
	_connection_lock::_connection_lock(std::shared_ptr<mongoc_client_t> c)
	: c_(c) {

	}
	_connection_lock& _connection_lock::exec(std::function<std::shared_ptr<bson_t> (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_error_t> error)> wk,
		std::function<void (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_t> d, std::shared_ptr<bson_error_t> error)> fn) {
		auto ptr = this->shared_from_this();
		// 工作线程
		boost::asio::post(controller_->context_ex, [this, wk, fn, ptr] () {
			std::shared_ptr<bson_error_t> error;
			// 执行命令
			std::shared_ptr<bson_t> r = wk(c_, error);
			// 后续流程 (回到主线程)
			boost::asio::post(context, [this, fn, r, error, ptr] () {
				fn(c_, r, error);
			});
		});
		return *this;
	}
}
}