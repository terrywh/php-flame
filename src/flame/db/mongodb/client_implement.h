#pragma once

namespace flame {
	class thread_worker;
namespace db {
namespace mongodb {
	class client;
	class client_implement {
	private:
		client_implement(client* cli);
		
		std::shared_ptr<thread_worker> worker_;
		client*                        client_;
		mongoc_client_t*                  cli_;
		mongoc_uri_t*                     uri_;
		
		// name -> connection_uri
		static void    connect_wk(uv_work_t* req);
		// name -> collection_name
		static void collection_wk(uv_work_t* req);
		static void      close_wk(uv_work_t* req);
		
		friend class client;
	};
	
	typedef struct client_request_t {
		coroutine*          co;
		client_implement* self;
		php::value          rv;
		php::string       name;
		uv_work_t          req;
	} client_request_t;
}
}
}
