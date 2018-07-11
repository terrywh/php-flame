#include "../coroutine.h"
#include "_connection_base.h"
#include "result.h"

namespace flame {
namespace mysql {
	void _connection_base::escape(php::buffer& b, const php::value& v, char quote) {
		switch(Z_TYPE_P(static_cast<zval*>(v))) {
		case IS_NULL:
			b.append("NULL", 4);
			break;
		case IS_TRUE:
			b.append("TRUE", 4);
			break;
		case IS_FALSE:
			b.append("FALSE", 5);
			break;
		// case IS_LONG:
		// case IS_DOUBLE: {
		// 	php::string str = v;
		// 	str.to_string();
		// 	b.append(str);
		// 	break;
		// }
		case IS_STRING: {
			php::string str = v;
			// 支持字段名 aaa.bbb 进行 ESCAPE 变为 `aaa`.`bbb`
			if(quote == '`') {
				const char* s = str.c_str(), *c, *e = str.c_str() + str.size();
				for(c = s; c < e; ++c) {
					if(*c == '.') {
						escape(b, php::string(s, c-s), quote);
						b.push_back('.');
						escape(b, php::string(c+1, e-c-1), quote);
						goto ESCAPE_FINISHED;
					}
				}
			}
			char* to = b.prepare(str.size() * 2 + 2);
			std::size_t n = 0;
			to[n++] = quote;
			if(s_ & SERVER_STATUS_NO_BACKSLASH_ESCAPES) { // 摘自 mysql_real_escape_string_quote() @ libmysql.c:1228 相关流程
				n += escape_quotes_for_mysql(i_, to + 1, str.size() * 2 + 1, str.c_str(), str.size(), quote);
			}else{
				n += escape_string_for_mysql(i_, to + 1, str.size() * 2 + 1, str.c_str(), str.size());
			}
			to[n++] = quote;
			b.commit(n);
			break;
		}
		case IS_ARRAY: {
			php::array arr = v;
			int index = 0;
			b.push_back('(');
			for(auto i=arr.begin();i!=arr.end();++i) {
				if(++index > 1) b.push_back(',');
				escape(b, i->second, quote);
			}
			b.push_back(')');
			break;
		}
		default: {
			php::string str = v;
			str.to_string();
			escape(b, str, quote);
		}
		}
ESCAPE_FINISHED:
		;		
	}
	void _connection_base::query(std::shared_ptr<coroutine> co, const php::object& ref, const php::string& sql) {
		exec([sql] (std::shared_ptr<MYSQL> c, int& error) -> std::shared_ptr<MYSQL_RES> { // 工作线程
			MYSQL* conn = c.get();
			error = mysql_real_query(conn, sql.c_str(), sql.size());
			if(error != 0) return nullptr;
			MYSQL_RES* r = mysql_store_result(conn); // 为了防止长时间锁表, 与 PHP 内部实现一致使用 store 二非 use
			if(!r && (error = mysql_field_count(conn)) != 0) return nullptr; // 应该但未返回 RESULT_SET
			return std::shared_ptr<MYSQL_RES>(r, mysql_free_result);
		}, [co, sql, ref] (std::shared_ptr<MYSQL> c, std::shared_ptr<MYSQL_RES> r, int error) { // 主线程 (需要继续捕捉 sql 保证其释放动作在主线程进行)
			MYSQL* conn = c.get();
			if(error) { // 错误
				co->fail(mysql_error(conn), mysql_errno(conn));
			}else{ 
				php::object rs(php::class_entry<result>::entry());
				if(r) { // 查询型返回
					rs.set("found_rows", static_cast<std::int64_t>(mysql_affected_rows(conn)));
					result* rs_ = static_cast<result*>(php::native(rs));
					rs_->r_ = r;
				}else{ // 更新型返回
					rs.set("affected_rows", static_cast<std::int64_t>(mysql_affected_rows(conn)));
					rs.set("insert_id", static_cast<std::int64_t>(mysql_insert_id(conn)));
				}
				co->resume(rs);
			}
		});
	}
}
}