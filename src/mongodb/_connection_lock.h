#pragma once

namespace flame {
namespace mongodb {
	class _connection_lock: public _connection_base, public std::enable_shared_from_this<_connection_lock> {
	public:
		// 以下函数应在主线程调用
		_connection_lock(std::shared_ptr<mongoc_client_t> c);
		virtual _connection_lock& exec(std::function<std::shared_ptr<bson_t> (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_error_t> error)> wk,
			std::function<void (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_t> d, std::shared_ptr<bson_error_t> error)> fn) override;
	private:
		std::shared_ptr<mongoc_client_t> c_;
	};
}
}