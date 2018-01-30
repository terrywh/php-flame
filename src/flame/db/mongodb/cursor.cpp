#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "collection.h"
#include "cursor_implement.h"
#include "cursor.h"
#include "mongodb.h"

namespace flame {
namespace db {
namespace mongodb {
	void cursor::init(thread_worker* worker, collection* col, mongoc_cursor_t* css) {
		impl = new cursor_implement(worker, this, css);
		ref_ = col;
	}
	php::value cursor::__destruct(php::parameters& params) {
		if(impl) {
			cursor_request_t* ctx = new cursor_request_t {
				nullptr, impl, php::array(nullptr)
			};
			ctx->req.data = ctx;
			// 这里不适用 close_work，实际停止动作在 client 中
			impl->worker_->queue_work(&ctx->req, cursor_implement::close_wk, close_cb);
		}
		return nullptr;
	}
	void cursor::close_cb(uv_work_t* req, int status) {
		delete reinterpret_cast<cursor_request_t*>(req->data);
	}
	void cursor::next_cb(uv_work_t* req, int status) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		if(ctx->idx == 0) {
			ctx->rv = php::array(0);
			fill_with(ctx->rv, ctx->doc);
		}
		ctx->co->next(std::move(ctx->rv));
		delete ctx;
	}
	void cursor::to_array_cb(uv_work_t* req, int status) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		if(ctx->idx == -2) { // 遍历启动
			// ctx->self ==> impl
			ctx->idx = -1;
			ctx->self->worker_->queue_work(&ctx->req, cursor_implement::to_array_wk, to_array_cb);
		}else if(ctx->idx == -1) {  // 遍历结束
			ctx->co->next(std::move(ctx->rv));
			delete ctx;
		}else if(ctx->idx >= 0) {
			php::array row(0);
			fill_with(row, ctx->doc);
			ctx->rv[ctx->idx] = std::move(row);
			// ctx->self ==> impl
			ctx->self->worker_->queue_work(&ctx->req, cursor_implement::to_array_wk, to_array_cb);
		}
	}
	php::value cursor::to_array(php::parameters& params) {
		cursor_request_t* ctx = new cursor_request_t {
			coroutine::current, impl, php::array(0), nullptr, -2
		};
		ctx->req.data = ctx;
		to_array_cb(&ctx->req, 0);
		return flame::async(this);
	}
	php::value cursor::next(php::parameters& params) {
		cursor_request_t* ctx = new cursor_request_t {
			coroutine::current, impl, php::array(nullptr), nullptr, -2
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, cursor_implement::next_wk, next_cb);
		return flame::async(this);
	}
}
}
}
