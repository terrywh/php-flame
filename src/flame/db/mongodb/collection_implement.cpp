#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "collection_implement.h"
#include "collection.h"
#include "mongodb.h"

namespace flame {
namespace db {
namespace mongodb {
	collection_implement::collection_implement(thread_worker* worker,
		collection* cpp, mongoc_collection_t* col)
	: worker_(worker)
	, cpp_(cpp)
	, col_(col) {
	
	}
	void collection_implement::count_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		bson_t filter;
		bson_init(&filter);
		fill_with(&filter, ctx->doc1);
		
		// 由于错误对象可能被传递到主线程使用，故需要在堆进行分配
		bson_error_t* error = new bson_error_t;
		int64_t c = mongoc_collection_count(
			ctx->self->col_, MONGOC_QUERY_SLAVE_OK, &filter, 0, 0, nullptr, error);
		bson_destroy(&filter);
		if(c == -1) {
			ctx->rv.ptr(error);
		}else{
			delete error;
			ctx->rv = c;
		}
	}
	void collection_implement::insert_one_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		bson_t filter;
		bson_init(&filter);
		fill_with(&filter, ctx->doc1);
		
		bson_error_t* error = new bson_error_t;
		bool r = mongoc_collection_insert(
			ctx->self->col_, MONGOC_INSERT_NONE, &filter, nullptr, error);
		bson_destroy(&filter);
		if(r) {
			ctx->rv = php::BOOL_YES;
			delete error;
		}else{
			ctx->rv.ptr(error);
		}
	}
	void collection_implement::insert_many_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		bson_t opts;
		bson_init(&opts);
		fill_with(&opts, ctx->doc2);
		// 构建 批量操作
		mongoc_bulk_operation_t* bulk = mongoc_collection_create_bulk_operation_with_opts(
			ctx->self->col_, &opts);
		bson_destroy(&opts);

		bson_t* reply,* doc;
		php::array& docs = ctx->doc1;
		for(auto i=docs.begin(); i!= docs.end(); ++i) {
			php::array& item = i->second;
			if(!item.is_array() || !item.is_a_map()) {
				continue;
			}
			doc = bson_new();
			fill_with(doc, item);
			mongoc_bulk_operation_insert(bulk, doc);
			bson_destroy(doc);
		}
		// 执行批量操作
		bson_error_t error;
		reply = bson_new();
		ctx->flags = mongoc_bulk_operation_execute(bulk, reply, &error);
		ctx->rv.ptr(reply); // 需要清理
		mongoc_bulk_operation_destroy(bulk);
	}
	void collection_implement::remove_one_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		bson_t filter;
		bson_init(&filter);
		fill_with(&filter, ctx->doc1);
		// 由于错误对象可能被传递到主线程使用，故需要在堆进行分配
		bson_error_t* error = new bson_error_t;
		bool r = mongoc_collection_remove(
			ctx->self->col_, MONGOC_REMOVE_SINGLE_REMOVE, &filter, nullptr, error);
		bson_destroy(&filter);
		if(r) {
			delete error;
			ctx->rv = php::BOOL_YES;
		}else{
			ctx->rv.ptr(error);
		}
	}
	void collection_implement::remove_many_wk(uv_work_t* w) {
		collection_request_t* ctx =
			reinterpret_cast<collection_request_t*>(w->data);
		bson_t filter;
		bson_init(&filter);
		fill_with(&filter, ctx->doc1);
		
		bson_error_t* error = new bson_error_t;
		bool r = mongoc_collection_remove(
			ctx->self->col_, MONGOC_REMOVE_NONE, &filter, nullptr, error);
		bson_destroy(&filter);
		if(r) {
			delete error;
			ctx->rv = php::BOOL_YES;
		}else{
			ctx->rv.ptr(error);
		}
	}
	void collection_implement::update_wk(uv_work_t* w) {
		collection_request_t* ctx =
			reinterpret_cast<collection_request_t*>(w->data);
		bson_t filter, update;
		bson_init(&filter);
		fill_with(&filter, ctx->doc1);
		bson_init(&update);
		fill_with(&update, ctx->doc2);
		
		bson_error_t* error = new bson_error_t;
		bool r = mongoc_collection_update(
			ctx->self->col_, (mongoc_update_flags_t)ctx->flags, &filter, &update,
			nullptr, error);
		bson_destroy(&filter);
		bson_destroy(&update);
		if(r) {
			delete error;
			ctx->rv = php::BOOL_YES;
		}else{
			ctx->rv.ptr(error);
		}
	}

	void collection_implement::find_many_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		bson_t filter, option;
		bson_init(&filter);
		fill_with(&filter, ctx->doc1);
		bson_init(&option);
		fill_with(&option, ctx->doc2);
		
		mongoc_cursor_t* cs = mongoc_collection_find_with_opts(
			ctx->self->col_, &filter, &option, nullptr);
		bson_destroy(&filter);
		bson_destroy(&option);
		
		// cursor 对象创建需要在主线程进行
		ctx->doc1.ptr(cs);
	}
	void collection_implement::find_one_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		bson_t filter, option;
		bson_init(&filter);
		fill_with(&filter, ctx->doc1);
		bson_init(&option);
		fill_with(&option, ctx->doc2);
		
		mongoc_cursor_t* cs = mongoc_collection_find_with_opts(
			ctx->self->col_, &filter, &option, nullptr);
		bson_destroy(&filter);
		bson_destroy(&option);
		
		const bson_t *doc;
		if(mongoc_cursor_next(cs, &doc)) {
			ctx->doc1.ptr((void*)doc);
			ctx->doc2.ptr(cs);
		}
		// 需要传递 find_one_af 进行销毁
		// mongoc_cursor_destroy(cs);
	}
	void collection_implement::find_one_af(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		mongoc_cursor_t* cs = ctx->doc2.ptr<mongoc_cursor_t>();
		mongoc_cursor_destroy(cs);
	}
	void collection_implement::close_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		mongoc_collection_destroy(ctx->self->col_);
	}
}
}
}
