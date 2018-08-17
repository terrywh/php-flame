#pragma once

namespace flame {
	class coroutine;
namespace mysql {
	class _connection_base {
	public:
		// 以下函数应在主线程调用
		_connection_base()
		:i_(nullptr) {};
		virtual ~_connection_base() {};
		typedef std::function<MYSQL_RES* (std::shared_ptr<MYSQL> c, int& error)> worker_fn_t;
		typedef std::function<void (std::shared_ptr<MYSQL> c, MYSQL_RES* r, int error)> master_fn_t;
		virtual _connection_base& exec(worker_fn_t&& wk, master_fn_t&& fn) = 0;
		void escape(php::buffer& b, const php::value& v, char quote = '\''); // 方便使用
		void query(std::shared_ptr<coroutine> co, const php::object& ref, const php::string& sql);
	protected:
		CHARSET_INFO* i_; // 为 escape 准备, 方便同步使用
		unsigned int  s_;
	};
}
}
