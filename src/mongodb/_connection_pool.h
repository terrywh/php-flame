#pragma once

namespace flame {
namespace mongodb {
	class _connection_pool: public _connection_base, public std::enable_shared_from_this<_connection_pool> {
	public:
		// 以下函数应在主线程调用
		_connection_pool(const php::string& url);
		~_connection_pool();
		virtual _connection_pool& exec(worker_fn_t&& wk, master_fn_t&& fn) override;
	private:

		mongoc_client_pool_t* pool_;
	};
}
}
