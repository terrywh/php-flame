#include "mysql.h"
#include "client.h"
#include "result_set.h"

namespace flame {
namespace db {
namespace mysql {
	void init(php::extension_entry& ext) {
		ext.add(php::constant_entry("flame\\db\\mysql\\FETCH_ASSOC", MYSQLND_FETCH_ASSOC));
		ext.add(php::constant_entry("flame\\db\\mysql\\FETCH_ARRAY", MYSQLND_FETCH_NUM));
		// ---------------------------------------------------------------------
		php::class_entry<client> class_client("flame\\db\\mysql\\client");
		class_client.add(php::property_entry("affected_rows", 0));
		class_client.add(php::property_entry("insert_id", 0));
		class_client.add<&client::__construct>("__construct");
		class_client.add<&client::__destruct>("__destruct");
		class_client.add<&client::close>("close");
		class_client.add<&client::connect>("connect");
		class_client.add<&client::format>("format");
		class_client.add<&client::query>("query");
		class_client.add<&client::insert>("insert");
		class_client.add<&client::remove>("delete");
		class_client.add<&client::update>("update");
		class_client.add<&client::select>("select");
		class_client.add<&client::one>("one");
		class_client.add<&client::found_rows>("found_rows");
		ext.add(std::move(class_client));
		// ---------------------------------------------------------------------
		php::class_entry<result_set> class_result_set("flame\\db\\mysql\\result_set");
		class_result_set.add<&result_set::fetch_array>("fetch_array");
		class_result_set.add<&result_set::fetch_assoc>("fetch_assoc");
		class_result_set.add<&result_set::fetch_all>("fetch_all");
		ext.add(std::move(class_result_set));
	}
	static void condition_item(client* cli, php::string& key, php::value& val, php::buffer& buf) {
		buf.add(' ');
		if(key.c_str()[0] != '$') { // 普通字段
			buf.add('`');
			std::memcpy(buf.put(key.length()), key.c_str(), key.length());
			buf.add('`');
			if(val.is_array()) {
				php::array& cond = val;
				if(cond.is_a_map()) {
					for(auto i=cond.begin();i!=cond.end();++i) {
						condition_item(cli, i->first, i->second, buf);
					}
				}else{ // IN 描述
					std::memcpy(buf.put(4), " IN ", 4);
					cli->value_to_buffer(val, buf);
				}
			}else if(val.is_null()) {
				std::memcpy(buf.put(7), "IS NULL", 7);
			}else {
				buf.add('=');
				cli->value_to_buffer(val, buf);
			}
			return;
		}
		if(std::strncmp(key.c_str(), "$and", 4) == 0 || std::strncmp(key.c_str(), "$AND", 4) == 0) {
			php::array& sub = val;
			int j = -1;
			buf.add('(');
			for(auto i=sub.begin();i!=sub.end();++i) {
				if(++j > 0) std::memcpy(buf.put(5), " AND", 5);
				condition_item(cli, i->first, i->second, buf);
			}
			buf.add(')');
		}else if(std::strncmp(key.c_str(), "$or", 3) == 0 || std::strncmp(key.c_str(), "$OR", 3) == 0) {
			php::array& sub = val;
			int j = -1;
			buf.add('(');
			for(auto i=sub.begin();i!=sub.end();++i) {
				if(++j > 0) std::memcpy(buf.put(4), " OR", 4);
				condition_item(cli, i->first, i->second, buf);
			}
			buf.add(')');
		}else if(std::strncmp(key.c_str(), "$gt", 3) == 0) {
			buf.add('>');
			cli->value_to_buffer(val, buf);
		}else if(std::strncmp(key.c_str(), "$gte", 4) == 0) {
			std::memcpy(buf.put(3), ">= ", 3);
			cli->value_to_buffer(val, buf);
		}else if(std::strncmp(key.c_str(), "$lt", 3) == 0) {
			buf.add('<');
			cli->value_to_buffer(val, buf);
		}else if(std::strncmp(key.c_str(), "$lte", 4) == 0) {
			std::memcpy(buf.put(3), "<= ", 3);
			cli->value_to_buffer(val, buf);
		}else if(std::strncmp(key.c_str(), "$ne", 3) == 0) {
			if(val.is_null()) {
				std::memcpy(buf.put(11), "IS NOT NULL", 11);
			}else{
				std::memcpy(buf.put(3), "!= ", 3);
				cli->value_to_buffer(val, buf);
			}
		}else if(std::strncmp(key.c_str(), "$in", 3) == 0) {
			std::memcpy(buf.put(3), "IN ", 3);
			cli->value_to_buffer(val, buf);
		}else if(std::strncmp(key.c_str(), "$like", 5) == 0) {
			std::memcpy(buf.put(5), "LIKE ", 5);
			cli->value_to_buffer(val, buf);
		}else{
			throw php::exception("illegal sql where");
		}
	}
	void sql_where(client* cli, php::value& data, php::buffer& buf) {
		std::memcpy(buf.put(6), " WHERE", 6);
		if(data.is_string()) {
			buf.add(' ');
			php::string& str = data;
			std::memcpy(buf.put(str.length()), str.data(), str.length());
			return;
		}
		php::array& cond = data;
		if(!cond.is_array() || !cond.is_a_map()) {
			throw php::exception("illegal sql where");
		}
		int j = -1;
		for(auto i=cond.begin(); i!= cond.end(); ++i) {
			if(++j > 0) {
				std::memcpy(buf.put(4), " AND", 4);
			}
			condition_item(cli, i->first, i->second, buf);
		}
	}
	void sql_orderby(client* cli, php::value& data, php::buffer& buf) {
		std::memcpy(buf.put(10), " ORDER BY ", 10);
		if(data.is_string()) {
			php::string& str = data;
			std::memcpy(buf.put(str.length()), str.c_str(), str.length());
		}else if(data.is_array()) {
			php::array& sort = data;
			int j = -1;
			for(auto i=sort.begin();i!=sort.end();++i) {
				php::string& key = i->first;
				int64_t dir = i->second.to_long();
				if(++j > 0) buf.add(',');
				if(dir > 0) {
					buf.add('`');
					std::memcpy(buf.put(key.length()), key.c_str(), key.length());
					std::memcpy(buf.put(5), "` ASC", 5);
				}else{
					buf.add('`');
					std::memcpy(buf.put(key.length()), key.c_str(), key.length());
					std::memcpy(buf.put(6), "` DESC", 6);
				}
			}
		}else{
			throw php::exception("illegl sql order by");
		}
	}
	void sql_limit(client* cli, php::value& data, php::buffer& buf) {
		std::memcpy(buf.put(7), " LIMIT ", 7);
		if(data.is_string()) {
			php::string& str = data;
			std::memcpy(buf.put(str.length()), str.data(), str.length());
		}else if(data.is_long()) {
			int64_t x = data;
			size_t  n = sprintf(buf.rev(10), "%ld", x);
			buf.adv(n);
		}else if(data.is_array()) {
			php::array& limit = data;
			int64_t x = limit[0], y = 0;
			if(limit.length() > 1) {
				y = limit[1];
			}
			if(y == 0) {
				size_t  n = sprintf(buf.rev(10), "%ld", x);
				buf.adv(n);
			}else{
				size_t  n = sprintf(buf.rev(20), "%ld,%ld", x, y);
				buf.adv(n);
			}
		}else{
			throw php::exception("illegal sql limit");
		}
	}
}
}
}
