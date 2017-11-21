#pragma once

#include "../../thread_worker.h"

namespace flame {
namespace db {
namespace mysql {
	class client: public php::class_base {
	public:
		void value_to_buffer(php::value& val, php::buffer& buf);

		client();
		php::value __destruct(php::parameters& params);
		php::value connect(php::parameters& params);
		php::value format(php::parameters& params);
		php::value query(php::parameters& params);
		php::value one(php::parameters& params);
		php::value insert(php::parameters& params);
		php::value remove(php::parameters& params);
		php::value update(php::parameters& params);
		php::value select(php::parameters& params);
		php::value close(php::parameters& params);
		php::value found_rows(php::parameters& params);
		// php::value autocommit(php::parameters& params);
		// php::value begin_transaction(php::parameters& params);
		// php::value commit(php::parameters& params);
		// php::value rollback(php::parameters& params);
	private:
		thread_worker worker_;
		void connect_();
		void query_(php::buffer& buf);
		MYSQLND* mysql_;
		std::shared_ptr<php_url> url_;
		static void connect_wk(uv_work_t* req);
		static void query_wk(uv_work_t* req);
		static void insert_wk(uv_work_t* req);
		static void one_wk(uv_work_t* req);
		static void found_rows_wk(uv_work_t* req);
		// static void autocommit_wk(uv_work_t* req);
		// static void begin_transaction_wk(uv_work_t* req);
		// static void commit_wk(uv_work_t* req);
		// static void rollback_wk(uv_work_t* req);
		void insert_key(php::array& map, php::buffer& buf);

		friend class result_set;
	};
}
}
}
