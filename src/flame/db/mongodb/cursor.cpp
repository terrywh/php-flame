#include "../../coroutine.h"
#include "cursor.h"
#include "mongodb.h"

namespace flame {
namespace db {
namespace mongodb {
	void cursor::init(const php::object& col, mongoc_cursor_t* cs) {
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
		php::callable cb;
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
			coroutine::current, this, this, nullptr
		};
		ctx->req.data = ctx;
		uv_queue_work(flame::loop, &ctx->req, next_wk, default_cb);
		return flame::async();
	}
	void cursor::to_array_wk(uv_work_t* req) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		php::array        arr;
		const bson_t*     doc;
		int               idx = -1;
		if(ctx->cb.is_callable()) {
			while(mongoc_cursor_next(ctx->self->cursor_, &doc)) {
				php::array row;
				fill_with(row, doc);
				if(ctx->cb(row).is_true()) {
					arr[++idx] = std::move(row);
					std::printf("[%d]\n", idx);
				}
			}
		}else{
			while(mongoc_cursor_next(ctx->self->cursor_, &doc)) {
				php::array row;
				fill_with(row, doc);
				arr[++idx] = std::move(row);
				std::printf("[%d]\n", idx);
			}
		}
		ctx->rv = std::move(arr);
	}
	php::value cursor::to_array(php::parameters& params) {
		cursor_request_t* ctx = new cursor_request_t {
			coroutine::current, this, this, nullptr
		};
		ctx->req.data = ctx;
		if(params.length() > 0 && params[0].is_callable()) {
			ctx->cb = params[0];
		}
		uv_queue_work(flame::loop, &ctx->req, to_array_wk, default_cb);
		return flame::async();
	}
}
}
}
