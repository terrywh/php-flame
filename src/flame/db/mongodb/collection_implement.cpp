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
		stack_bson_t filter(ctx->doc1);
		
		collection_response_t* res = new collection_response_t {
			RETURN_VALUE_TYPE_ERROR,
		};
		int64_t c = mongoc_collection_count(ctx->self->col_, MONGOC_QUERY_SLAVE_OK, filter, 0, 0, nullptr, &res->error);

		if(c == -1) {
			ctx->rv.ptr(res);	
		}else{
			delete res;
			ctx->rv = c;
		}
	}
// #define EXECUTE_AND_RETURN_VALUE(stm) if(stm) { \
// 	res->type = RETURN_VALUE_TYPE_REPLY;        \
// } else {                                        \
// 	res->type = RETURN_VALUE_TYPE_ERROR;        \
// }                                               \
// ctx->rv.ptr(res);
	void collection_implement::insert_one_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		stack_bson_t document(ctx->doc1), option(ctx->doc2);

		collection_response_t* res = new collection_response_t;
		if(mongoc_collection_insert_one(ctx->self->col_, document, option, &res->reply, &res->error)) {
			res->type = RETURN_VALUE_TYPE_REPLY;
		}else{
			res->type = RETURN_VALUE_TYPE_ERROR;
		}
		ctx->rv.ptr(res);
	}
	void collection_implement::insert_many_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		php::array& doc1 = ctx->doc1;
		bson_t* docs[doc1.length()];
		int docn = 0;
		for(auto i=doc1.begin(); i!=doc1.end(); ++i) {
			php::array& item = i->second;
			if(!item.is_array() || !item.is_a_map()) {
				continue;
			}
			docs[docn] = bson_new();
			fill_with(docs[docn], item);
			++docn;
		}
		stack_bson_t opts(ctx->doc2);
		collection_response_t* res = new collection_response_t;
		if(mongoc_collection_insert_many(ctx->self->col_, (const bson_t**)docs, docn, opts, &res->reply, &res->error)) {
			res->type = RETURN_VALUE_TYPE_REPLY;
		}else{
			res->type = RETURN_VALUE_TYPE_ERROR;
		}
		ctx->rv.ptr(res);
		for(auto i=0; i<docn; ++i) {
			bson_destroy(docs[i]);
		}
	}
	void collection_implement::remove_one_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		stack_bson_t filter(ctx->doc1), option(ctx->doc2);

		collection_response_t* res = new collection_response_t;
		if(mongoc_collection_delete_one(ctx->self->col_, filter, option, &res->reply, &res->error)) {
			res->type = RETURN_VALUE_TYPE_REPLY;
		}else{
			res->type = RETURN_VALUE_TYPE_ERROR;
		}
		ctx->rv.ptr(res);
	}
	void collection_implement::remove_many_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		stack_bson_t filter(ctx->doc1), option(ctx->doc2);

		collection_response_t* res = new collection_response_t;
		if(mongoc_collection_delete_many(ctx->self->col_, filter, option, &res->reply, &res->error)) {
			res->type = RETURN_VALUE_TYPE_REPLY;
		}else{
			res->type = RETURN_VALUE_TYPE_ERROR;
		}
		ctx->rv.ptr(res);
	}
	void collection_implement::update_one_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		stack_bson_t filter(ctx->doc1), update(ctx->doc2), option(ctx->doc3);

		collection_response_t* res = new collection_response_t;
		if(mongoc_collection_update_one(ctx->self->col_, filter, update, option, &res->reply, &res->error)) {
			res->type = RETURN_VALUE_TYPE_REPLY;
		}else{
			res->type = RETURN_VALUE_TYPE_ERROR;
		}
		ctx->rv.ptr(res);
	}
	void collection_implement::update_many_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		stack_bson_t filter(ctx->doc1), update(ctx->doc2), option(ctx->doc3);

		collection_response_t* res = new collection_response_t;
		if(mongoc_collection_update_many(ctx->self->col_, filter, update, option, &res->reply, &res->error)) {
			res->type = RETURN_VALUE_TYPE_REPLY;
		}else{
			res->type = RETURN_VALUE_TYPE_ERROR;
		}
		ctx->rv.ptr(res);
	}
	void collection_implement::find_many_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		stack_bson_t filter(ctx->doc1), option(ctx->doc2);

		mongoc_cursor_t* cs = mongoc_collection_find_with_opts(ctx->self->col_, filter, option, nullptr);
		
		// cursor 对象创建需要在主线程进行
		ctx->doc1.ptr(cs);
	}
	void collection_implement::find_one_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		stack_bson_t filter(ctx->doc1), option(ctx->doc2);
		
		mongoc_cursor_t* cs = mongoc_collection_find_with_opts(ctx->self->col_, filter, option, nullptr);
		
		const bson_t *doc;
		if(mongoc_cursor_next(cs, &doc)) {
			ctx->doc1.ptr((void*)doc);
			ctx->doc2.ptr(cs);
		}
		// 由于实际文档 doc 引用指针 cs 中相关数据，故需要回到子线程进行销毁：find_one_af
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
