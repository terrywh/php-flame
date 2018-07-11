#pragma once

namespace flame {
namespace mysql {
	class _connection_pool: public _connection_base, public std::enable_shared_from_this<_connection_pool> {
	public:
		// 以下函数应在主线程调用
		_connection_pool(std::shared_ptr<php::url> url, std::size_t max = 4);
		~_connection_pool();
		virtual _connection_pool& exec(std::function<std::shared_ptr<MYSQL_RES> (std::shared_ptr<MYSQL> c, int& error)> wk,
			std::function<void (std::shared_ptr<MYSQL> c, std::shared_ptr<MYSQL_RES> r, int error)> fn) override;
		
	private:
		// 以下函数应在工作线程调用
		void acquire(std::function<void (std::shared_ptr<MYSQL> c)> cb);
		void release(MYSQL* c);
		
		std::shared_ptr<php::url> url_;
		std::uint16_t      max_;
		std::uint16_t     size_;
		boost::asio::io_context::strand                         wait_guard; // 防止对下面队列操作发生多线程问题;
		std::list<std::function<void (std::shared_ptr<MYSQL>)>> wait_;
		std::list<MYSQL*>                                       conn_;
	};
}
}