#pragma once

namespace flame {
	class thread_worker;
namespace db {
namespace mongodb {
	class collection;
	class cursor_implement;
	class cursor: public php::class_base {
	public:
		cursor();
		void init(thread_worker* worker, collection* col, mongoc_cursor_t* cs);
		php::value __construct(php::parameters& params) {
			return nullptr;
		}
		php::value __destruct(php::parameters& params);
		php::value to_array(php::parameters& params);
		php::value next(php::parameters& params);
		
		cursor_implement* impl;
	private:
		php::object       ref_;
		static void close_cb(uv_work_t* req, int status);
		static void next_cb(uv_work_t* req, int status);
		static void to_array_cb(uv_work_t* req, int status);
	};
}
}
}
