#include "../controller.h"
#include "../coroutine.h"
#include "mysql.h"
#include "_connection_base.h"
#include "_connection_pool.h"
#include "transaction.h"
#include "client.h"
#include "result.h"

namespace flame {
namespace mysql {
	void declare(php::extension_entry& ext) {
		ext
			.on_module_startup([] (php::extension_entry& ext) -> bool {
				return mysql_library_init(0, nullptr, nullptr) == 0;
			})
			.on_module_shutdown([] (php::extension_entry& ext) -> bool {
				mysql_library_end();
				return true;
			});
		ext
			.function<connect>("flame\\mysql\\connect");
		transaction::declare(ext);
		client::declare(ext);
		result::declare(ext);
	}
	php::value connect(php::parameters& params) {
		php::object cli(php::class_entry<client>::entry());
		client* cli_ = static_cast<client*>(php::native(cli));

		cli_->c_.reset(new _connection_pool(php::parse_url(params[0])));
		std::shared_ptr<coroutine> co = coroutine::current;
		cli_->c_->exec([] (std::shared_ptr<MYSQL> c, int& error) -> std::shared_ptr<MYSQL_RES> {
			return nullptr;
		}, [co, cli] (std::shared_ptr<MYSQL> c, std::shared_ptr<MYSQL_RES> rs, int error) {
			co->resume(cli);
		});
		return coroutine::async();
	}
	// 相等
	static void where_eq(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& cond) {
		if(cond.typeof(php::TYPE::NULLABLE)) {
			buf.append(" IS NULL", 8);
		}else{
			php::string str = cond;
			str.to_string();
			buf.push_back('=');
			cc->escape(buf, str);
		}
	}
	// 不等 {!=}
	static void where_ne(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& cond) {
		if(cond.typeof(php::TYPE::NULLABLE)) {
			buf.append(" IS NOT NULL", 12);
		}else{
			php::string str = cond;
			str.to_string();
			buf.append("!=",2);
			cc->escape(buf, str);
		}
	}
	// 大于 {>}
	static void where_gt(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& cond) {
		if(cond.typeof(php::TYPE::NULLABLE)) {
			buf.append(">0", 2);
		}else{
			php::string str = cond;
			str.to_string();
			buf.push_back('>');
			cc->escape(buf, str);
		}
	}
	// 小于 {<}
	static void where_lt(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& cond) {
		if(cond.typeof(php::TYPE::NULLABLE)) {
			buf.append("<0", 2);
		}else{
			php::string str = cond;
			str.to_string();
			buf.push_back('<');
			cc->escape(buf, str);
		}
	}
	// 大于等于 {>=}
	static void where_gte(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& cond) {
		if(cond.typeof(php::TYPE::NULLABLE)) {
			buf.append(">=0", 3);
		}else{
			php::string str = cond;
			str.to_string();
			buf.append(">=", 2);
			cc->escape(buf, str);
		}
	}
	// 小于等于 {<=}
	static void where_lte(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& cond) {
		if(cond.typeof(php::TYPE::NULLABLE)) {
			buf.append("<=0", 3);
		}else{
			php::string str = cond;
			str.to_string();
			buf.append("<=", 2);
			cc->escape(buf, str);
		}
	}
 	// 某个 IN 无对应符号
	static void where_in(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::array& cond) {
		assert(cond.typeof(php::TYPE::ARRAY) && "目标格式错误");

		buf.append(" IN (", 5);
		for(auto i=cond.begin(); i!=cond.end(); ++i) {
			if(static_cast<int>(i->first) > 0) {
				buf.push_back(',');
			}
			php::string v = i->second;
			v.to_string();
			cc->escape(buf, v);
		}
		buf.push_back(')');
	}
	// WITH IN RANGE
	static void where_wir(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::array& cond) {
		assert(cond.typeof(php::TYPE::ARRAY) && "目标格式错误");
		buf.append(" BETWEEN ", 9);
		std::int64_t min = std::numeric_limits<std::int64_t>::max(), max = std::numeric_limits<std::int64_t>::min();
		for(auto i=cond.begin(); i!=cond.end(); ++i) {
			std::int64_t x = i->second.to_integer();
			if(x > max) max = x;
			if(x < min) min = x;
		}
		buf.append(std::to_string(min));
		buf.append(" AND ", 5);
		buf.append(std::to_string(max));
	}
	// OUT OF RANGE
	static void where_oor(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::array& cond) {
		buf.append(" NOT", 4);
		where_wir(cc, buf, cond);
	}
	// LINK
	static void where_lk(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& cond) {
		buf.append(" LIKE ", 6);
		cc->escape(buf, cond);
	}
	// NOT LIKE
	static void where_nlk(std::shared_ptr<_connection_base> cc, php::buffer& buf,const php::value& cond) {
		buf.append(" NOT LIKE ", 10);
		cc->escape(buf, cond);
	}
	static void where_ex(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::array& cond, const php::string& field, const php::string& separator) {
		if(cond.size() > 1) buf.push_back('(');

		int j = -1;
		for(auto i=cond.begin(); i!=cond.end(); ++i) {
			if(++j > 0) buf.append(separator);

			if(i->first.typeof(php::TYPE::INTEGER)) {
				if(i->second.typeof(php::TYPE::ARRAY)) {
					where_ex(cc, buf, i->second, i->first, " && ");
				}else{
					php::string str = i->second;
					str.to_string();
					buf.append(str);
				}
			}else {
				php::string key = i->first;
				if(key.c_str()[0] == '{') { // OPERATOR (php::TYPE::STRING)
					if((key.size() == 5 && strncasecmp(key.c_str(), "{NOT}", 5) == 0) || (key.size() == 3 && strncasecmp(key.c_str(), "{!}", 3) == 0)) {
						assert(i->second.typeof(php::TYPE::ARRAY));
						buf.append(" NOT ", 5);
						where_ex(cc, buf, i->second, php::string(nullptr), " && ");
					}else if(key.size() == 4 && (strncasecmp(key.c_str(), "{OR}", 4) == 0 || strncasecmp(key.c_str(), "{||}", 4) == 0)) {
						assert(i->second.typeof(php::TYPE::ARRAY));
						where_ex(cc, buf, i->second, php::string(nullptr), " || ");
					}else if((key.size() == 5 && strncasecmp(key.c_str(), "{AND}", 5) == 0) || (key.size() == 4 && strncasecmp(key.c_str(), "{&&}", 4) == 0)) {
						assert(i->second.typeof(php::TYPE::ARRAY));
						where_ex(cc, buf, i->second, php::string(nullptr), " && ");
					}else if(key.size() == 4 && strncasecmp(key.c_str(), "{!=}", 4) == 0) {
						cc->escape(buf, field, '`');
						where_ne(cc, buf, i->second);
					}else if(key.size() == 3 && strncasecmp(key.c_str(), "{>}", 3) == 0) {
						cc->escape(buf, field, '`');
						where_gt(cc, buf, i->second);
					}else if(key.size() == 3 && strncasecmp(key.c_str(), "{<}", 2) == 0) {
						cc->escape(buf, field, '`');
						where_lt(cc, buf, i->second);
					}else if(key.size() == 4 && strncasecmp(key.c_str(), "{>=}", 4) == 0) {
						cc->escape(buf, field, '`');
						where_gte(cc, buf, i->second);
					}else if(key.size() == 4 && strncasecmp(key.c_str(), "{<=}", 4) == 0) {
						cc->escape(buf, field, '`');
						where_lte(cc, buf, i->second);
					}else if(key.size() == 4 && strncasecmp(key.c_str(), "{<>}", 4) == 0) {
						cc->escape(buf, field, '`');
						where_wir(cc, buf, i->second);
					}else if(key.size() == 4 && strncasecmp(key.c_str(), "{><}", 4) == 0) {
						cc->escape(buf, field, '`');
						where_oor(cc, buf, i->second);
					}else if(key.size() == 3 && strncasecmp(key.c_str(), "{~}", 3) == 0) {
						cc->escape(buf, field, '`');
						where_lk(cc, buf, i->second);
					}else if(key.size() == 4 && strncasecmp(key.c_str(), "{!~}", 4) == 0) {
						cc->escape(buf, field, '`');
						where_nlk(cc, buf, i->second);
					}

				}else{ // php::TYPE::STRING
					if(i->second.typeof(php::TYPE::ARRAY)) {
						php::array cond = i->second;
						if(cond.exists(0)) {
							cc->escape(buf, key, '`');
							buf.push_back(' ');
							where_in(cc, buf, i->second);
						}else{
							where_ex(cc, buf, cond, key, " && ");
						}
					}else{
						cc->escape(buf, key, '`');
						where_eq(cc, buf, i->second);
					}
				}
			}
		}
		if(cond.size() > 1) buf.push_back(')');
	}
	void build_where(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& data) {
		buf.append(" WHERE ", 7);
		if(data.typeof(php::TYPE::STRING) || data.typeof(php::TYPE::INTEGER)) {
			buf.push_back(' ');
			buf.append(data);
		}else if(data.typeof(php::TYPE::ARRAY)) {
			where_ex(cc, buf, data, php::string(0), " && ");
		}else{
			buf.push_back('1');
		}
	}
	void build_order(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& data) {
		if(data.typeof(php::TYPE::STRING)) {
			buf.append(" ORDER BY ", 10);
			buf.append(data);
		}else if(data.typeof(php::TYPE::ARRAY)) {
			buf.append(" ORDER BY", 9);
			php::array order = data;
			int j = -1;
			for(auto i=order.begin(); i!=order.end(); ++i) {
				if(++j > 0) buf.push_back(',');
				if(i->first.typeof(php::TYPE::INTEGER)) {
					if(i->second.typeof(php::TYPE::STRING)) {
						buf.push_back(' ');
						buf.append(i->second);
					}
				}else{
					buf.push_back(' ');
					cc->escape(buf, i->first, '`');
					if(i->second.typeof(php::TYPE::STRING)) {
						buf.push_back(' ');
						buf.append(i->second);
					}else if(i->second.typeof(php::TYPE::YES) || (i->second.typeof(php::TYPE::INTEGER) && static_cast<int>(i->second) >= 0)) {
						buf.append(" ASC", 4);
					}else{
						buf.append(" DESC", 5);
					}
				}
			}
		}else{
			// throw php::exception(zend_ce_type_error, "failed to build 'ORDER BY' clause: unsupported type");
		}
	}
	void build_limit(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& data) {
		if(data.typeof(php::TYPE::STRING) || data.typeof(php::TYPE::INTEGER)) {
			buf.append(" LIMIT ", 7);
			buf.append(data);
		}else if(data.typeof(php::TYPE::ARRAY)) {
			buf.append(" LIMIT", 6);
			php::array limit = data;
			int j = -1;
			for(auto i=limit.begin(); i!=limit.end(); ++i) {
				if(++j > 0) buf.push_back(',');
				buf.push_back(' ');
				buf.append(i->second);
				if(j > 0) break;
			}
		}else{
			throw php::exception(zend_ce_type_error, "failed to build 'LIMIT' clause: unsupported type");
		}
	}
	static void insert_row(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::array& row) {
		int j = -1;
		for(auto i=row.begin(); i!=row.end(); ++i) {
			if(++j > 0) buf.append(", ", 2);
			cc->escape(buf, i->second);
		}
	}
	void build_insert(std::shared_ptr<_connection_base> cc, php::buffer& buf, php::parameters& params) {
		buf.append("INSERT INTO ", 12);
		// 表名
		cc->escape(buf, params[0], '`');
		// 字段
		buf.push_back('(');
		php::array rows = params[1], row;
		if(rows.exists(0)) {
			row  = rows.get(0);
			assert(!row.exists(0) && "二级行无字段");
		}else{
			row  = rows;
			rows = nullptr;
		}
		assert(row.typeof(php::TYPE::ARRAY) && "行非数组");
		int j = -1;
		for(auto i=row.begin(); i!=row.end(); ++i) {
			if(++j > 0) buf.append(", ", 2);
			cc->escape(buf, i->first, '`');
		}
		// 数据
		buf.append(") VALUES", 8);
		if(rows.typeof(php::TYPE::ARRAY)) {
			buf.push_back('(');
			int j = -1;
			for(auto i=rows.begin(); i!=rows.end(); ++i) {
				if(++j > 0) buf.append("), (", 4);
				insert_row(cc, buf, i->second);
			}
			buf.push_back(')');
		}else{
			buf.push_back('(');
			insert_row(cc, buf, row);
			buf.push_back(')');
		}
	}
	void build_delete(std::shared_ptr<_connection_base> cc, php::buffer& buf, php::parameters& params) {
		buf.append("DELETE FROM ", 12);
		// 表名
		cc->escape(buf, params[0], '`');
		// 条件
		build_where(cc, buf, params[1]);
		// 排序
		if(params.size() > 2) build_order(cc, buf, params[2]);
		// 限制
		if(params.size() > 3) build_limit(cc, buf, params[2]);
	}
	void build_update(std::shared_ptr<_connection_base> cc, php::buffer& buf, php::parameters& params) {
		buf.append("UPDATE ", 7);
		// 表名
		cc->escape(buf, params[0], '`');
		// 数据
		buf.append(" SET", 4);
		php::array data = params[2];
		if(data.typeof(php::TYPE::STRING)) {
			buf.push_back(' ');
			buf.append(data);
		}else if(data.typeof(php::TYPE::ARRAY)) {
			int j=-1;
			for(auto i=data.begin(); i!=data.end(); ++i) {
				if(++j > 0) buf.push_back(',');
				buf.push_back(' ');
				cc->escape(buf, i->first, '`');
				buf.push_back('=');
				cc->escape(buf, i->second);
			}
		}else{
			throw php::exception(zend_ce_type_error, "failed to build 'UPDATE' clause: unsupported type");
		}
		// 条件
		build_where(cc, buf, params[1]);
		// 排序
		if(params.size() > 3) build_order(cc, buf, params[3]);
		// 限制
		if(params.size() > 4) build_limit(cc, buf, params[4]);
	}
	void build_select(std::shared_ptr<_connection_base> cc, php::buffer& buf, php::parameters& params) {
		buf.append("SELECT ", 7);
		// 字段
		php::value fields = params[1];
		if(fields.typeof(php::TYPE::STRING)) {
			buf.append(fields);
		}else if(fields.typeof(php::TYPE::ARRAY)) {
			php::array a = fields;
			int j = -1;
			for(auto i=a.begin(); i!=a.end(); ++i) {
				if(++j>0) buf.append(", ", 2);
				if(i->first.typeof(php::TYPE::INTEGER)) {
					cc->escape(buf, i->second, '`');
				}else{
					buf.append(i->first);
					buf.push_back('(');
					cc->escape(buf, i->second, '`');
					buf.push_back(')');
				}
			}
		}else{
			throw php::exception(zend_ce_type_error, "failed to build 'SELECT' clause: unsupported type");
		}
		// 表名
		buf.append(" FROM ", 6);
		cc->escape(buf, params[0], '`');
		// 条件
		if(params.size() > 2) build_where(cc, buf, params[2]);
		// 排序
		if(params.size() > 3) build_order(cc, buf, params[3]);
		// 限制
		if(params.size() > 4) build_limit(cc, buf, params[4]);
	}
	void build_one(std::shared_ptr<_connection_base> cc, php::buffer& buf, php::parameters& params) {
		buf.append("SELECT *  FROM ", 15);
		// 表名
		cc->escape(buf, params[0], '`');
		// 条件
		if(params.size() > 1) build_where(cc, buf, params[1]);
		// 排序
		if(params.size() > 2) build_order(cc, buf, params[2]);
		buf.append(" LIMIT 1", 8);
	}
	void build_get(std::shared_ptr<_connection_base> cc, php::buffer& buf, php::parameters& params) {
		buf.append("SELECT ", 7);
		// 字段
		if(params[1].typeof(php::TYPE::STRING)) {
			cc->escape(buf, params[1], '`');
		}else{
			throw php::exception(zend_ce_type_error, "failed to build 'SELECT' clause: unsupported type");
		}
		// 表名
		buf.append(" FROM ", 6);
		cc->escape(buf, params[0], '`');
		// 条件
		if(params.size() > 2) build_where(cc, buf, params[2]);
		// 排序
		if(params.size() > 3) build_order(cc, buf, params[3]);
		buf.append(" LIMIT 1", 8);
	}
	void fetch_cb(std::shared_ptr<coroutine> co, const php::object& ref, const php::string& field) {
		co->stack(php::value([field] (php::parameters& params) -> php::value {
			php::object cs = params[0];
			assert(cs.instanceof(php::class_entry<result>::entry()));
			if(static_cast<int>(cs.get("found_rows")) > 0) {
				php::array row = cs.call("fetch_row");
				if(field.typeof(php::TYPE::STRING)) return row.get(field);
				else return std::move(row);
			}else{
				return nullptr;
			}
		}), ref);
	}
}
}
