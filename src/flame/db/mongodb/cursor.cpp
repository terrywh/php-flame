#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "collection.h"
#include "cursor_implement.h"
#include "cursor.h"
#include "mongodb.h"

namespace flame {
namespace db {
namespace mongodb {
	cursor::cursor()
	:impl(nullptr) {

	}
	void cursor::init(std::shared_ptr<thread_worker> worker, collection* col, mongoc_cursor_t* css) {
		impl = new cursor_implement(worker, this, css);
		ref_ = col;
	}
	cursor::~cursor() {
		if(impl) {
			cursor_request_t* ctx = new cursor_request_t {
				nullptr, impl, nullptr
			};
			ctx->req.data = ctx;
			impl->worker_->queue_work(&ctx->req, cursor_implement::close_wk, close_cb);
		}
	}
	void cursor::close_cb(uv_work_t* req, int status) {
		delete reinterpret_cast<cursor_request_t*>(req->data);
	}
	void cursor::next_cb(uv_work_t* req, int status) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		if(ctx->idx == 0) {
			const bson_t* doc = ctx->ref.ptr<const bson_t>();
			ctx->rv = php::array(0);
			fill_with(ctx->rv, doc);
		}
		ctx->co->next(std::move(ctx->rv));
		delete ctx;
	}
	void cursor::to_array_cb(uv_work_t* req, int status) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		if(ctx->idx == -1) {  // 遍历结束
			ctx->co->next(std::move(ctx->rv));
			delete ctx;
			return; // 遍历结束
		}
		
		if(ctx->idx == -2) {
			++ctx->idx;
		}else if(ctx->idx >= 0) {
			const bson_t* doc = ctx->ref.ptr<const bson_t>();
			php::array row(0);
			fill_with(row, doc);
			ctx->rv[ctx->idx] = std::move(row);
		}
		// ctx->self ==> impl
		ctx->self->worker_->queue_work(&ctx->req, cursor_implement::to_array_wk, to_array_cb);
	}
	php::value cursor::to_array(php::parameters& params) {
		cursor_request_t* ctx = new cursor_request_t {
			coroutine::current, impl, this, -2, php::array(0)
		};
		ctx->req.data = ctx;
		to_array_cb(&ctx->req, 0);
		return flame::async();
	}
	php::value cursor::next(php::parameters& params) {
		cursor_request_t* ctx = new cursor_request_t {
			coroutine::current, impl, this, 0, php::array(nullptr)
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, cursor_implement::next_wk, next_cb);
		return flame::async();
	}
}
}
}
