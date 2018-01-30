#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "mysql.h"
#include "result_implement.h"

#define MYSQL_PING_INTERVAL 10000
namespace flame {
namespace db {
namespace mysql {
	result_implement::result_implement(thread_worker* worker, MYSQL_RES* rs)
	: worker_(worker)
	, rs_(rs) {
		
	}
	void result_implement::fetch_row_wk(uv_work_t* req) {
		result_request_t* ctx = reinterpret_cast<result_request_t*>(req->data);
		if(ctx->type == MYSQL_FETCH_NUM) {
			sql_fetch_row(ctx->self->rs_, ctx->rv, MYSQL_FETCH_NUM);
		}else/* if(ctx->type == MYSQL_FETCH_ASSOC) */{
			sql_fetch_row(ctx->self->rs_, ctx->rv, MYSQL_FETCH_ASSOC);
		}
	}
	void result_implement::fetch_all_wk(uv_work_t* req) {
		result_request_t* ctx = reinterpret_cast<result_request_t*>(req->data);
		sql_fetch_all(ctx->self->rs_, ctx->rv, ctx->type == MYSQL_FETCH_NUM ? MYSQL_FETCH_NUM : MYSQL_FETCH_ASSOC);
	}
	
	void result_implement::close_wk(uv_work_t* req) {
		result_request_t* ctx = reinterpret_cast<result_request_t*>(req->data);
		mysql_free_result(ctx->self->rs_);
		delete ctx->self;
	}
}
}
}
