#include "../../coroutine.h"
#include "result_implement.h"

#define MYSQL_PING_INTERVAL 10000
namespace flame {
namespace db {
namespace mysql {
	result_implement::result_implement(thread_worker* worker, MYSQLND_RES* rs)
	: worker_(worker)
	, rs_(rs) {
		
	}
	void result_implement::fetch_row_wk(uv_work_t* req) {
		result_request_t* ctx = reinterpret_cast<result_request_t*>(req->data);
		zval rv;
		if(ctx->type == MYSQLND_FETCH_NUM) {
			mysqlnd_fetch_into(ctx->self->rs_, MYSQLND_FETCH_NUM, &rv, MYSQLND_MYSQLI);
		}else/* if(ctx->type == MYSQLND_FETCH_ASSOC) */{
			mysqlnd_fetch_into(ctx->self->rs_, MYSQLND_FETCH_ASSOC, &rv, MYSQLND_MYSQLI);
		}
		ctx->rv = php::value(rv);
	}
	void result_implement::fetch_all_wk(uv_work_t* req) {
		result_request_t* ctx = reinterpret_cast<result_request_t*>(req->data);
		php::array rv(0);
		// php::value rs;
		zval rs;
		int i = 0;
		do {
			mysqlnd_fetch_into(ctx->self->rs_, ctx->type == MYSQLND_FETCH_NUM ? MYSQLND_FETCH_NUM : MYSQLND_FETCH_ASSOC, &rs, MYSQLND_MYSQLI);
			// if(!rs.is_array()) {
			if(Z_TYPE(rs) != IS_ARRAY) {
				// rs = nullptr; // 参考 mysqlnd_result.c:1797
				zval_ptr_dtor(&rs);
				break;
			}
			rv[i++] = php::value(rs);
			// rs = nullptr;
		}while(1);

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
