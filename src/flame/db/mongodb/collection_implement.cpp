#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "collection_implement.h"
#include "collection.h"
#include "mongodb.h"
#include "bulk_result.h"
#include "cursor.h"


namespace flame {
namespace db {
namespace mongodb {
	collection_implement::collection_implement(std::shared_ptr<thread_worker> worker,
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
		
		bson_error_t error;
		int64_t c = mongoc_collection_count(
			ctx->self->col_, MONGOC_QUERY_SLAVE_OK,
			&filter, 0, 0, nullptr, &error);
		bson_destroy(&filter);
		if(c == -1) {
			ctx->rv = php::make_exception(error.message, error.code);
		}else{
			ctx->rv = c;
		}
	}
	void collection_implement::insert_one_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		bson_t filter;
		bson_init(&filter);
		fill_with(&filter, ctx->doc1);
		
		bson_error_t error;
		bool r = mongoc_collection_insert(
			ctx->self->col_, MONGOC_INSERT_NONE,
			&filter, nullptr, &error);
		bson_destroy(&filter);
		if(r) {
			ctx->rv = bool(true);
		}else{
			ctx->rv = php::make_exception(error.message, error.code);
		}
	}
	void collection_implement::insert_many_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		// 构建 批量操作
		mongoc_bulk_operation_t* bulk = mongoc_collection_create_bulk_operation(
			ctx->self->col_, ctx->flags, nullptr);
			
		bson_t reply,* doc;
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
		uint32_t r = mongoc_bulk_operation_execute(bulk, &reply, &error);
		if(r == 0) {
			php::warn("insert_many failed: (%d) %s", error.code, error.message);
			ctx->rv = bulk_result::create_from(false, &reply);
		}else{
			ctx->rv = bulk_result::create_from(true, &reply);
		}
		bson_destroy(&reply);
		mongoc_bulk_operation_destroy(bulk);
	}
	void collection_implement::remove_one_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		bson_t filter;
		bson_init(&filter);
		fill_with(&filter, ctx->doc1);
		
		bson_error_t error;
		bool r = mongoc_collection_remove(
			ctx->self->col_,
			MONGOC_REMOVE_SINGLE_REMOVE,
			&filter,
			nullptr,
			&error);
		bson_destroy(&filter);
		if(r) {
			ctx->rv = true;
		}else{
			ctx->rv = php::make_exception(error.message, error.code);
		}
	}
	void collection_implement::remove_many_wk(uv_work_t* w) {
		collection_request_t* ctx =
			reinterpret_cast<collection_request_t*>(w->data);
		bson_t filter;
		bson_init(&filter);
		fill_with(&filter, ctx->doc1);
		
		bson_error_t error;
		bool r = mongoc_collection_remove(
			ctx->self->col_, MONGOC_REMOVE_NONE, &filter, nullptr, &error);
		bson_destroy(&filter);
		if(r) {
			ctx->rv = true;
		}else{
			ctx->rv = php::make_exception(error.message, error.code);
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
		
		bson_error_t error;
		bool r = mongoc_collection_update(
			ctx->self->col_, (mongoc_update_flags_t)ctx->flags, &filter, &update,
			nullptr, &error);
		bson_destroy(&filter);
		bson_destroy(&update);
		if(r) {
			ctx->rv = true;
		}else{
			ctx->rv = php::make_exception(error.message, error.code);
		}
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
		
		const bson_t* doc;
		ctx->rv = php::array(0);
		while (mongoc_cursor_next(cs, &doc)) {
			ctx->rv = php::array(0);
			fill_with(ctx->rv, doc); // 这里的 doc 不须释放
		}
		// 释放实际在 cursor 内部
		mongoc_cursor_destroy(cs);
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
		
		php::object obj = php::object::create<cursor>();
		cursor*     cpp = obj.native<cursor>();
		cpp->init(ctx->self->worker_, ctx->self->cpp_, cs);
		ctx->rv = std::move(obj);
	}
	void collection_implement::close_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		mongoc_collection_destroy(ctx->self->col_);
	}
}
}
}
