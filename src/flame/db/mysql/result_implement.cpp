#include "../../coroutine.h"
#include "result_implement.h"

#define MYSQL_PING_INTERVAL 10000
namespace flame {
namespace db {
namespace mysql {
	result_implement::result_implement(std::shared_ptr<thread_worker> worker, MYSQLND_RES* rs)
	: worker_(worker)
	, rs_(rs) {
		
	}
	void result_implement::fetch_row_wk(uv_work_t* req) {
		result_request_t* ctx = reinterpret_cast<result_request_t*>(req->data);
		if(ctx->type == MYSQLND_FETCH_NUM) {
			mysqlnd_fetch_into(ctx->self->rs_, MYSQLND_FETCH_NUM, (zval*)&ctx->rv, MYSQLND_MYSQLI);
		}else/* if(ctx->type == MYSQLND_FETCH_ASSOC) */{
			mysqlnd_fetch_into(ctx->self->rs_, MYSQLND_FETCH_ASSOC, (zval*)&ctx->rv, MYSQLND_MYSQLI);
		}
	}
	void result_implement::fetch_all_wk(uv_work_t* req) {
		result_request_t* ctx = reinterpret_cast<result_request_t*>(req->data);
		
		php::array rv(0);
		php::value rs;
		mysqlnd_fetch_into(ctx->self->rs_, ctx->type == MYSQLND_FETCH_NUM ? MYSQLND_FETCH_NUM : MYSQLND_FETCH_ASSOC, (zval*)&rs, MYSQLND_MYSQLI);
		for(int i=0; rs.is_array(); ++i) {
			rv[i] = std::move(rs);
			mysqlnd_fetch_into(ctx->self->rs_, ctx->type == MYSQLND_FETCH_NUM ? MYSQLND_FETCH_NUM : MYSQLND_FETCH_ASSOC, (zval*)&rs, MYSQLND_MYSQLI);
		}
		ctx->rv = std::move(rv);
	}
	
	void result_implement::close_wk(uv_work_t* req) {
		result_request_t* ctx = reinterpret_cast<result_request_t*>(req->data);
		mysqlnd_free_result(ctx->self->rs_, true);
		delete ctx->self;
	}
}
}
}
