#pragma once

namespace flame {
	class thread_worker;
namespace db {
namespace mysql {
	class client;
	struct client_request_t;
	class client_implement {
	private:
		client_implement(client* cli);
		thread_worker         worker_;
		client*               client_;
		MYSQL                  mysql_;
		bool                   debug_;
		bool               connected_;
		uv_timer_t              ping_;
		client_request_t*       ping_context;

		void start();
		static void ping_cb(uv_timer_t* req);
		static void ping_cb(uv_work_t* req, int status);
		void destroy();
		static void destroy_cb(uv_handle_t* handle);
		// sql -> connection_uri
		static void    connect_wk(uv_work_t* req);
		static void       ping_wk(uv_work_t* req);
		static void      query_wk(uv_work_t* req);
		static void     insert_wk(uv_work_t* req);
		static void        one_wk(uv_work_t* req);
		// sql -> SELECT FOUND_ROWS()
		static void found_rows_wk(uv_work_t* req);
		static void    destroy_wk(uv_work_t* req);
		static void    destroy_cb(uv_work_t* req, int status);

		static void begin_wk(uv_work_t* req);
		static void commit_wk(uv_work_t* req);
		static void rollback_wk(uv_work_t* req);
		
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
