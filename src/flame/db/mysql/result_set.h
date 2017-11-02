#pragma once

namespace flame {
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
		php::object  client_ref;
		MYSQLND_RES* rs_;
		void init(php::object ref, MYSQLND_RES* rs);
		static void array_wk(uv_work_t* req);
		static void assoc_wk(uv_work_t* req);
		static void all1_wk(uv_work_t* req);
		static void all2_wk(uv_work_t* req);
		friend class client;
	};
}
}
}
