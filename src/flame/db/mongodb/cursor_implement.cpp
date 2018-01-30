#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "cursor_implement.h"
#include "cursor.h"
#include "mongodb.h"

namespace flame {
namespace db {
namespace mongodb {
	cursor_implement::cursor_implement(thread_worker* worker, cursor* c, mongoc_cursor_t* cs)
	:worker_(worker)
	,cpp_(c)
	,css_(cs) {

	}
	void cursor_implement::next_wk(uv_work_t* req) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		const bson_t*     doc;
		if(mongoc_cursor_next(ctx->self->css_, &doc)) {
			// 由于数据行中需要构建 object_id / date_time 对象，须在主线程进行
			ctx->idx = 0;
			ctx->doc = doc;
		}else{
			ctx->idx = -1;
		}
	}
	void cursor_implement::to_array_wk(uv_work_t* req) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		const bson_t*     doc;
		if(mongoc_cursor_next(ctx->self->css_, &doc)) {
			// 由于数据行中需要构建 object_id / date_time 对象，须在主线程进行
			++ctx->idx;
			ctx->doc = doc;
		}else{
			ctx->idx = -1;
		}
	}
	void cursor_implement::close_wk(uv_work_t* req) {
		cursor_request_t* ctx = reinterpret_cast<cursor_request_t*>(req->data);
		mongoc_cursor_destroy(ctx->self->css_);
	}
}
}
}
