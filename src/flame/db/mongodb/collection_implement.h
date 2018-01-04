#pragma once

namespace flame {
	class thread_worker;
namespace db {
namespace mongodb {
	class collection;
	class collection_implement {
	public:
		collection_implement(thread_worker* worker, collection* cpp,
			mongoc_collection_t* col);
	private:
		thread_worker*       worker_;
		collection*             cpp_;
		mongoc_collection_t*    col_;
		
		static void count_wk(uv_work_t* w);
		static void insert_one_wk(uv_work_t* w);
		static void insert_many_wk(uv_work_t* w);
		static void remove_one_wk(uv_work_t* w);
		static void remove_many_wk(uv_work_t* w);
		static void update_wk(uv_work_t* w);
		static void find_one_wk(uv_work_t* w);
		static void find_one_af(uv_work_t* w);
		static void find_many_wk(uv_work_t* w);
		static void close_wk(uv_work_t* w);
		friend class collection;
	};
	
	typedef struct collection_request_t {
		coroutine*    co;
		collection_implement* self;
		php::value    rv; // 异步启动时作为引用，返回时作为结果
		php::array  doc1;
		php::array  doc2;
		int         flags;
		uv_work_t   req;
	} collection_request_t;
}
}
}
