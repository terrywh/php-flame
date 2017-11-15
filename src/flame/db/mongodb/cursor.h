#pragma once

namespace flame {
	class thread_worker;
namespace db {
namespace mongodb {
	class collection;
	class cursor: public php::class_base {
	public:
		cursor();
		~cursor();
		php::value to_array(php::parameters& params);
		php::value next(php::parameters& params);
	private:
		thread_worker*   worker_;
		php::value       collection_object;
		mongoc_cursor_t* cursor_;
		int              cursor_index;
		void init(collection* col, mongoc_cursor_t* cs);

		static void next_wk(uv_work_t* req);
		static void to_array_wk(uv_work_t* req);
		static void default_cb(uv_work_t* req, int status);

		friend class collection;
	};
}
}
}
