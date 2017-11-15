#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "collection.h"
#include "cursor.h"
#include "mongodb.h"

namespace flame {
namespace db {
namespace mongodb {
	void cursor::init(collection* col, mongoc_cursor_t* cs) {
		worker_ = col->worker_;
		collection_object = col;
		cursor_ = cs;
	}
	cursor::cursor() : cursor_(nullptr) {

	}
	cursor::~cursor() {
		if(cursor_) mongoc_cursor_destroy(cursor_);
	}
	typedef struct cursor_request_t {
		coroutine*    co;
		cursor*     self;
		php::value    rv; // 引用 + 返回
		uv_work_t    req;
	} cursor_request_t;
	void cursor::default_cb(uv_work_t* req, int status) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		ctx->co->next(ctx->rv);
		delete ctx;
	}
	void cursor::next_wk(uv_work_t* req) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		const bson_t*     doc;
		if(mongoc_cursor_next(ctx->self->cursor_, &doc)) {
			php::array row;
			fill_with(row, doc);
			ctx->rv = std::move(row);
		}else{
			ctx->rv = (bool)false;
		}
	}
	php::value cursor::next(php::parameters& params) {
		cursor_request_t* ctx = new cursor_request_t {
			coroutine::current, this, this
		};
		ctx->req.data = ctx;
		worker_->queue_work(&ctx->req, next_wk, default_cb);
		return flame::async();
	}
	void cursor::to_array_wk(uv_work_t* req) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		php::array        arr;
		const bson_t*     doc;
		int               idx = -1;
		while(mongoc_cursor_next(ctx->self->cursor_, &doc)) {
			php::array row;
			fill_with(row, doc);
			arr[++idx] = std::move(row);
		}
		ctx->rv = std::move(arr);
	}
	php::value cursor::to_array(php::parameters& params) {
		cursor_request_t* ctx = new cursor_request_t {
			coroutine::current, this, this
		};
		ctx->req.data = ctx;
		worker_->queue_work(&ctx->req, to_array_wk, default_cb);
		return flame::async();
	}
}
}
}
