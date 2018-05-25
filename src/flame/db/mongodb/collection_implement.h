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
		const mongoc_read_prefs_t* get_read_prefs(php::array& opts);
		
		static void count_wk(uv_work_t* w);
		static void insert_one_wk(uv_work_t* w);
		static void insert_many_wk(uv_work_t* w);
		static void remove_one_wk(uv_work_t* w);
		static void remove_many_wk(uv_work_t* w);
		static void update_one_wk(uv_work_t* w);
		static void update_many_wk(uv_work_t* w);
		static void find_one_wk(uv_work_t* w);
		static void find_one_af(uv_work_t* w);
		static void find_many_wk(uv_work_t* w);
		static void close_wk(uv_work_t* w);
		static void aggregate_wk(uv_work_t* w);
		friend class collection;
	};
	
	typedef struct collection_request_t {
		coroutine*    co;
		collection_implement* self;
		php::value    rv; // 异步启动时作为引用，返回时作为结果
		php::array  doc1;
		php::array  doc2;
		php::array  doc3;
		uv_work_t   req;
	} collection_request_t;
	typedef enum collection_response_type_t {
			RETURN_VALUE_TYPE_REPLY,
			RETURN_VALUE_TYPE_ERROR,
	} collection_response_type_t;
	typedef struct collection_response_t {
		collection_response_type_t type;
		bson_error_t error;
		bson_t       reply;
		~collection_response_t() {
			bson_destroy(&reply);
		}
	} collection_response_t;
}
}
}
