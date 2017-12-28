#pragma once

#include "../../thread_worker.h"

namespace flame {
namespace db {
namespace mysql {

	class result_implement {
	private:
		result_implement(std::shared_ptr<thread_worker> worker, MYSQLND_RES* rs);
		
		
		std::shared_ptr<thread_worker> worker_;
		MYSQLND_RES*                       rs_;
		
		static void fetch_row_wk(uv_work_t* req);
		static void fetch_all_wk(uv_work_t* req);
		static void close_wk(uv_work_t* req);
		static void close_cb(uv_handle_t* req);
	
		friend class result_set;
	};
	
	typedef struct result_request_t {
		coroutine*          co;
		result_implement* self;
		php::value          rv;
		int               type;
		uv_work_t          req;
	} implement_request_t;
}
}
}
