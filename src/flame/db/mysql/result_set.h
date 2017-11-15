#pragma once

namespace flame {
	class thread_worker;
namespace db {
namespace mysql {
	class client;
	class result_set: public php::class_base {
	public:
		~result_set();
		php::value fetch_array(php::parameters& params);
		php::value fetch_assoc(php::parameters& params);
		php::value fetch_all(php::parameters& params);
	private:
		thread_worker* worker_;
		php::value     client_object;
		MYSQLND_RES*   rs_;
		void init(client* cli, MYSQLND_RES* rs);
		static void array_wk(uv_work_t* req);
		static void assoc_wk(uv_work_t* req);
		static void all1_wk(uv_work_t* req);
		static void all2_wk(uv_work_t* req);

		friend class client;
	};
}
}
}
