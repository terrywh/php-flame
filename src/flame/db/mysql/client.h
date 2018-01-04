#pragma once

namespace flame {
namespace db {
namespace mysql {
	class client_implement;
	class client: public php::class_base {
	public:
		void val_to_buffer(php::value& val, php::buffer& buf);
		void key_to_buffer(php::array& map, php::buffer& buf);

		php::value __construct(php::parameters& params);
		php::value __destruct(php::parameters& params);
		php::value connect(php::parameters& params);
		php::value format(php::parameters& params);
		php::value query(php::parameters& params);
		php::value one(php::parameters& params);
		php::value insert(php::parameters& params);
		php::value remove(php::parameters& params);
		php::value update(php::parameters& params);
		php::value select(php::parameters& params);
		php::value found_rows(php::parameters& params);

		php::value begin_transaction(php::parameters& params);
		php::value commit(php::parameters& params);
		php::value rollback(php::parameters& params);
		
		client_implement* impl;
	private:
		void query_(const php::string& sql);
		static void connect_cb(uv_work_t* req, int status);
		static void queue_cb(uv_work_t* req, int status);
		static void default_cb(uv_work_t* req, int status);
		int        ping_interval;
		static void ping_cb(uv_timer_t* req);
		static void ping_cb(uv_work_t* req, int status);
		static void destroy_cb(uv_handle_t* handle);

		friend class client_implement;
	};
}
}
}
