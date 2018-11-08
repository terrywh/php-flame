#include "../coroutine.h"
#include "transaction.h"
#include "_connection_base.h"
#include "_connection_lock.h"
#include "result.h"
#include "mysql.h"

namespace flame {
namespace mysql {
	void transaction::declare(php::extension_entry& ext) {
		php::class_entry<transaction> class_transaction("flame\\mysql\\transaction");
		class_transaction
			.method<&transaction::__construct>("__construct", {}, php::PRIVATE)
			.method<&transaction::escape>("escape")
			.method<&transaction::query>("query")
			.method<&transaction::insert>("insert")
			.method<&transaction::delete_>("delete")
			.method<&transaction::update>("update")
			.method<&transaction::select>("select")
			.method<&transaction::one>("one")
			.method<&transaction::get>("get")
			.method<&transaction::commit>("commit")
			.method<&transaction::rollback>("rollback");
		ext.add(std::move(class_transaction));
	}
	transaction::transaction() {
		
	}
	transaction::~transaction() {

	}
	php::value transaction::commit(php::parameters& params) {
		std::shared_ptr<coroutine> co = coroutine::current;
		// c_->query(coroutine::current, php::object(this), php::string("COMMIT", 6));
		c_->exec([] (std::shared_ptr<MYSQL> c, int& error) -> MYSQL_RES* { // 工作线程
			MYSQL* conn = c.get();
			error = mysql_real_query(conn, "COMMIT", 6);
			assert(mysql_field_count(conn) == 0);
			return nullptr;
		}, [co] (std::shared_ptr<MYSQL> c, MYSQL_RES* r, int error) { // 主线程
			MYSQL* conn = c.get();
			if(error) {
				co->fail(mysql_error(conn), mysql_errno(conn));
			}else{
				co->resume();
			}
		});
		return coroutine::async();
	}
	php::value transaction::rollback(php::parameters& params) {
		std::shared_ptr<coroutine> co = coroutine::current;
		// c_->query(coroutine::current, php::object(this), php::string("ROLLBACK", 8));
		c_->exec([] (std::shared_ptr<MYSQL> c, int& error) -> MYSQL_RES* { // 工作线程
			MYSQL* conn = c.get();
			error = mysql_real_query(conn, "ROLLBACK", 8);
			assert(mysql_field_count(conn) == 0);
			return nullptr;
		}, [co] (std::shared_ptr<MYSQL> c, MYSQL_RES*, int error) { // 主线程
			MYSQL* conn = c.get();
			if(error) {
				co->fail(mysql_error(conn), mysql_errno(conn));
			}else{
				co->resume();
			}
		});
		return coroutine::async();
	}
	php::value transaction::escape(php::parameters& params) {
		php::buffer b;
		c_->escape(b, params[0]);
		return std::move(b);
	}
	php::value transaction::query(php::parameters& params) {
		c_->query(coroutine::current, php::object(this), params[0]);
		return coroutine::async();
	}
	php::value transaction::insert(php::parameters& params) {
		php::buffer buf;
		build_insert(c_, buf, params);
		c_->query(coroutine::current, php::object(this), std::move(buf));
		return coroutine::async();
	}
	php::value transaction::delete_(php::parameters& params) {
		php::buffer buf;
		build_delete(c_, buf, params);
		c_->query(coroutine::current, php::object(this), std::move(buf));
		return coroutine::async();
	}
	php::value transaction::update(php::parameters& params) {
		php::buffer buf;
		build_update(c_, buf, params);
		c_->query(coroutine::current, php::object(this), std::move(buf));
		return coroutine::async();
	}
	php::value transaction::select(php::parameters& params) {
		php::buffer buf;
		build_select(c_, buf, params);
		c_->query(coroutine::current, php::object(this), std::move(buf));
		return coroutine::async();
	}
	php::value transaction::one(php::parameters& params) {
		php::buffer buf;
		build_one(c_, buf, params);
		result::stack_fetch(coroutine::current, php::object(this), php::string(nullptr));
		c_->query(coroutine::current, php::object(this), std::move(buf));
		return coroutine::async();
	}
	php::value transaction::get(php::parameters& params) {
		php::buffer buf;
		build_get(c_, buf, params);
		result::stack_fetch(coroutine::current, php::object(this), params[1]);
		c_->query(coroutine::current, php::object(this), std::move(buf));
		return coroutine::async();
	}
}
}
