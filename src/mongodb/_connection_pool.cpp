#include "../controller.h"
#include "../coroutine.h"
#include "_connection_base.h"
#include "_connection_pool.h"

namespace flame {
namespace mongodb {
	_connection_pool::_connection_pool(const php::string& url) {
		std::unique_ptr<mongoc_uri_t, void (*)(mongoc_uri_t*)> uri ( mongoc_uri_new(url.c_str()), mongoc_uri_destroy);
		if(!uri) {
			throw php::exception(zend_ce_type_error, "cannot connect to mysql server");
		}
		pool_ = mongoc_client_pool_new(uri.get());
	}
	_connection_pool::~_connection_pool() {
		mongoc_client_pool_destroy(pool_);
	}
	_connection_pool& _connection_pool::exec(std::function<std::shared_ptr<bson_t> (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_error_t> error)> wk,
		std::function<void (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_t> d, std::shared_ptr<bson_error_t> error)> fn) {
		auto ptr = this->shared_from_this();
		// 工作线程
		boost::asio::post(controller_->context_ex, [this, wk, fn, ptr] () {
			// 获取连接
			std::shared_ptr<mongoc_client_t> c(mongoc_client_pool_pop(pool_), [this, ptr] (mongoc_client_t* c) {
				boost::asio::post(controller_->context_ex, [this, c, ptr] () {
					mongoc_client_pool_push(pool_, c);
				});
			});
			std::shared_ptr<bson_error_t> error;
			// 执行命令
			std::shared_ptr<bson_t> r = wk(c, error);
			// 后续流程 (回到主线程)
			boost::asio::post(context, [this, fn, r, c, error, ptr] () {
				fn(c, r, error);
			});
		});
		return *this;
	}
}
}