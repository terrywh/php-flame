#pragma once

namespace flame {
namespace mysql {
	class _connection_lock: public _connection_base, public std::enable_shared_from_this<_connection_lock> {
	public:
		// 以下函数应在工作线程调用
		_connection_lock(std::shared_ptr<MYSQL> c);
		~_connection_lock();
		// 以下函数应在主线程调用
		virtual _connection_lock& exec(worker_fn_t&& wk, master_fn_t&& fn) override;
	private:
		std::shared_ptr<MYSQL> c_;
	};
}
}
