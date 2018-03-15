#pragma once

namespace flame {
	class thread_worker;
namespace db {
namespace mysql {
	class client;
	class result_implement;
	class result_set: public php::class_base {
	public:
		result_set();
		~result_set();
		
		void init(thread_worker* worker, client* cli, MYSQL_RES* rs);
		
		php::value fetch_row(php::parameters& params);
		php::value fetch_all(php::parameters& params);
		
		result_implement* impl;
	private:
		php::object ref_;
		static void default_cb(uv_work_t* req, int status);
	};
	class result_info: public php::class_base {
	public:
		void init(int64_t affected_rows, int64_t insert_id);
	};
}
}
}
