#include "../coroutine.h"
#include "result.h"
#include "_connection_base.h"
#include "_connection_lock.h"

namespace flame {
namespace mysql {
	void result::declare(php::extension_entry& ext) {
		php::class_entry<result> class_result("flame\\mysql\\result");
		class_result
			.property({"affected_rows", 0})
			.property({"insert_id", 0})
			.method<&result::fetch_row>("fetch_row")
			.method<&result::fetch_all>("fetch_all");

		ext.add(std::move(class_result));
	}
	void result::stack_fetch(std::shared_ptr<coroutine> co, const php::object& ref, const php::string& field) {
		co->stack(php::value([field] (php::parameters& params) -> php::value {
			php::array row = params[0];
			if(!row.typeof(php::TYPE::ARRAY)) return nullptr;
			if(field.typeof(php::TYPE::STRING)) return row.get(field);
			else return std::move(row);
		}))->stack(php::value([field] (php::parameters& params) -> php::value {
			php::object rs = params[0];
			result* rs_ = static_cast<result*>(php::native(rs));

			assert(rs.instanceof(php::class_entry<result>::entry()));
			if(rs_->n_ > 0) {
				return rs.call("fetch_row");
			}else{
				return nullptr;
			}
		}), ref);
	}
	result::~result() {
		if(r_) release();
	}
	struct fetch_row_t {
		MYSQL_ROW      data;
		unsigned long* size;
	};
	void result::fetch_next(std::shared_ptr<coroutine> co, const php::object& ref, bool fetch_all, php::array& data_all) {
		c_->exec([this] (std::shared_ptr<MYSQL> c, int& error) -> MYSQL_RES* {
			fetch_row_t* row = new fetch_row_t();
			assert(row->data == nullptr);
			if ((row->data = mysql_fetch_row(r_))) {
				row->size = mysql_fetch_lengths(r_);
			}
			return reinterpret_cast<MYSQL_RES*>(row);
		}, [this, co, ref, fetch_all, data_all] (std::shared_ptr<MYSQL> c, MYSQL_RES* r, int error) mutable {
			MYSQL* conn = c.get();
			fetch_row_t* row = reinterpret_cast<fetch_row_t*>(r);
			if(row->data == nullptr && mysql_errno(conn) != 0) {
				co->fail(mysql_error(conn), mysql_errno(conn));
				release(); // fetch 过程终止终止清理
			} else if(row->data == nullptr) {
				if(fetch_all) co->resume(data_all);
				else co->resume(nullptr);
				release(); // fetch 过程终止清理
			} else {
				php::array data_row((int)n_);
				for(int i = 0; i < n_; i++) {
					data_row.set(php::string(f_[i].name), php::string(row->data[i], row->size[i]));
				}
				if(fetch_all) {
					data_all.set(data_all.size(), data_row);
					fetch_next(co, ref, fetch_all, data_all);
				} else {
					co->resume(std::move(data_row));
				}
			}
			delete row;
		});
	}
	php::value result::fetch_row(php::parameters& params) {
		if(!r_) return nullptr;
		php::array data_all;
		fetch_next(coroutine::current, php::object(this), false, data_all);
		return coroutine::async();
	}
	php::value result::fetch_all(php::parameters& params) {
		if(!r_) return nullptr;
		php::array data_all(8);
		fetch_next(coroutine::current, php::object(this), true, data_all);
		return coroutine::async();
	}
	void result::release() {
		std::shared_ptr<_connection_lock> cl = c_;
		MYSQL_RES* r = r_;
		c_.reset();
		r_ = nullptr;
		f_ = nullptr;
		n_ = 0;
		cl->exec([r] (std::shared_ptr<MYSQL> c, int& error) -> MYSQL_RES* {
			mysql_free_result(r);
			return nullptr;
		}, [cl] (std::shared_ptr<MYSQL> c, MYSQL_RES* r, int error) {
			// cl.reset()
		});
	}
}
}
