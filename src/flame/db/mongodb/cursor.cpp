#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "collection.h"
#include "cursor_implement.h"
#include "cursor.h"

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
			impl->worker_->queue_work(&ctx->req, cursor_implement::close_wk, default_cb);
		}
	}
	void cursor::default_cb(uv_work_t* req, int status) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		if(ctx->co) ctx->co->next(ctx->rv);
		delete ctx;
	}
	php::value cursor::next(php::parameters& params) {
		cursor_request_t* ctx = new cursor_request_t {
			coroutine::current, impl, this
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, cursor_implement::next_wk, default_cb);
		return flame::async();
	}
	php::value cursor::to_array(php::parameters& params) {
		cursor_request_t* ctx = new cursor_request_t {
			coroutine::current, impl, this
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, cursor_implement::to_array_wk, default_cb);
		return flame::async();
	}
}
}
}
