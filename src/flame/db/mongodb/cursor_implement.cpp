#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "cursor_implement.h"
#include "cursor.h"
#include "mongodb.h"

namespace flame {
namespace db {
namespace mongodb {
	cursor_implement::cursor_implement(std::shared_ptr<thread_worker> worker, cursor* c, mongoc_cursor_t* cs)
	:worker_(worker)
	,cpp_(c)
	,css_(cs) {

	}
	void cursor_implement::next_wk(uv_work_t* req) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		const bson_t*     doc;
		if(mongoc_cursor_next(ctx->self->css_, &doc)) {
			php::array row;
			fill_with(row, doc);
			ctx->rv = std::move(row);
		}else{
			ctx->rv = (bool)false;
		}
	}
	void cursor_implement::to_array_wk(uv_work_t* req) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		php::array        rv(0);
		const bson_t*     doc;
		int               idx = -1;
		while(mongoc_cursor_next(ctx->self->css_, &doc)) {
			php::array row(0);
			fill_with(row, doc);
			rv[++idx] = std::move(row);
		}
		ctx->rv = std::move(rv);
	}
	void cursor_implement::close_wk(uv_work_t* req) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		mongoc_cursor_destroy(ctx->self->css_);
	}
}
}
}
