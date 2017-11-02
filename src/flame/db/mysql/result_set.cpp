#include "../../coroutine.h"
#include "result_set.h"

namespace flame {
namespace db {
namespace mysql {
	void result_set::init(php::object ref, MYSQLND_RES* rs) {
		client_ref = std::move(ref);
		rs_ = rs;
	}
	result_set::~result_set() {
		mysqlnd_free_result(rs_, false);
	}
	typedef struct fetch_request_t {
		coroutine*    co;
		result_set* self;
		php::value    rv;
		uv_work_t    req;
	} fetch_request_t;
	static void default_cb(uv_work_t* req, int status) {
		fetch_request_t* ctx = reinterpret_cast<fetch_request_t*>(req->data);
		ctx->co->next(ctx->rv);
		delete ctx;
	}
	void result_set::array_wk(uv_work_t* req) {
		fetch_request_t* ctx = reinterpret_cast<fetch_request_t*>(req->data);
		mysqlnd_fetch_into(ctx->self->rs_, MYSQLND_FETCH_NUM, (zval*)&ctx->rv, MYSQLND_MYSQLI);
	}
	php::value result_set::fetch_array(php::parameters& params) {
		fetch_request_t* ctx = new fetch_request_t {
			coroutine::current, this, this
		};
		ctx->req.data = ctx;
		uv_queue_work(flame::loop, &ctx->req, array_wk, default_cb);
		return flame::async();
	}
	void result_set::assoc_wk(uv_work_t* req) {
		fetch_request_t* ctx = reinterpret_cast<fetch_request_t*>(req->data);
		mysqlnd_fetch_into(ctx->self->rs_, MYSQLND_FETCH_ASSOC, (zval*)&ctx->rv, MYSQLND_MYSQLI);
	}
	php::value result_set::fetch_assoc(php::parameters& params) {
		fetch_request_t* ctx = new fetch_request_t {
			coroutine::current, this, this
		};
		ctx->req.data = ctx;
		uv_queue_work(flame::loop, &ctx->req, assoc_wk, default_cb);
		return flame::async();
	}
	void result_set::all1_wk(uv_work_t* req) {
		fetch_request_t* ctx = reinterpret_cast<fetch_request_t*>(req->data);
		php::array rst(4);
		php::value row;
		int        idx = 0;
		do {
			mysqlnd_fetch_into(ctx->self->rs_, MYSQLND_FETCH_ASSOC, (zval*)&row, MYSQLND_MYSQLI);
			if(!row.is_array()) break;
			rst[idx++] = std::move(row);
		} while(1);
		ctx->rv = std::move(rst);
	}
	void result_set::all2_wk(uv_work_t* req) {
		fetch_request_t* ctx = reinterpret_cast<fetch_request_t*>(req->data);
		php::array rst(4);
		php::value row;
		int        idx = 0;
		do {
			mysqlnd_fetch_into(ctx->self->rs_, MYSQLND_FETCH_NUM, (zval*)&row, MYSQLND_MYSQLI);
			if(!row.is_array()) break;
			rst[idx++] = std::move(row);
		} while(1);
		ctx->rv = std::move(rst);
	}
	php::value result_set::fetch_all(php::parameters& params) {
		uv_work_cb wk;
		if(params.length() == 1 && params[0].to_long() == MYSQLND_FETCH_NUM) {
			wk = all2_wk;
		}else{
			wk = all1_wk;
		}
		fetch_request_t* ctx = new fetch_request_t {
			coroutine::current, this, this
		};
		ctx->req.data = ctx;
		uv_queue_work(flame::loop, &ctx->req, wk, default_cb);
		return flame::async();
	}
}
}
}
