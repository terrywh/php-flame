#pragma once

namespace flame {
	class thread_worker;
namespace db {
namespace mysql {
	class client;
	class client_implement {
	private:
		client_implement(std::shared_ptr<thread_worker> worker, client* cli);
		void close();
		std::shared_ptr<thread_worker> worker_;
		client*               client_;
		MYSQLND*               mysql_;
		bool                   debug_;
		uv_timer_t              ping_;
		int                     ping_interval;
		std::shared_ptr<php_url> url_;
		bool               connected_;
		// sql -> connection_uri
		static void    connect_wk(uv_work_t* req);
		static void      query_wk(uv_work_t* req);
		static void     insert_wk(uv_work_t* req);
		static void        one_wk(uv_work_t* req);
		// sql -> SELECT FOUND_ROWS()
		static void found_rows_wk(uv_work_t* req);
		static void       ping_cb(uv_timer_t* req);
		static void      close_wk(uv_work_t* req);
		static void      close_cb(uv_handle_t* req);
		
		friend class client;
	};
	
	typedef struct client_request_t {
		coroutine*          co;
		client_implement* self;
		php::value          rv;
		php::string        sql;
		uv_work_t          req;
	} client_request_t;
	
}
}
}
