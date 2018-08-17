#pragma once

namespace flame {
namespace mysql {
	class _connection_lock;
	class result: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		static void stack_fetch(std::shared_ptr<coroutine> co, const php::object& ref, const php::string& field);
		~result();
		void fetch_next(std::shared_ptr<coroutine> co, const php::object& ref, bool fetch_all, php::array& data_all);
		php::value fetch_row(php::parameters& params);
		php::value fetch_all(php::parameters& params);
	private:
		std::shared_ptr<_connection_lock> c_;
		MYSQL_RES*   r_;
		MYSQL_FIELD* f_;
		unsigned int n_; // Number Of Fields
		void release();
		friend class _connection_base;
	};
}
}
