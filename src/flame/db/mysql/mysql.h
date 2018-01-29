#pragma once

namespace flame {
namespace db {
namespace mysql {
	class client;

	void init(php::extension_entry& ext);
	void sql_where(client* cli, php::value& cond, php::buffer& buf);
	void sql_orderby(client* cli, php::value& sort, php::buffer& buf);
	void sql_limit(client* cli, php::value& limit, php::buffer& buf);
	enum {
		MYSQL_FETCH_ASSOC,
		MYSQL_FETCH_NUM,
	};
	void sql_fetch_row(MYSQL_RES* rs, php::array& row, int flag);
	void sql_fetch_all(MYSQL_RES* rs, php::array& rv, int flag);
}
}
}
