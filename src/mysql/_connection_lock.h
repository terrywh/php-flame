#pragma once

namespace flame {
namespace mysql {
	class _connection_lock: public _connection_base, public std::enable_shared_from_this<_connection_lock> {
	public:
		// 以下函数应在工作线程调用
		_connection_lock(std::shared_ptr<MYSQL> c);
		// 以下函数应在主线程调用
		virtual _connection_lock& exec(std::function<std::shared_ptr<MYSQL_RES> (std::shared_ptr<MYSQL> c, int& error)> wk,
			std::function<void (std::shared_ptr<MYSQL> c, std::shared_ptr<MYSQL_RES> r, int error)> fn) override;
	private:
		std::shared_ptr<MYSQL> c_;
	};
}
}