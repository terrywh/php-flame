#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "../../time/time.h"
#include "mysql.h"
#include "client.h"
#include "client_implement.h"

namespace flame {
namespace db {
namespace mysql {
	client_implement::client_implement(client* cli)
	: client_(cli)
	, debug_(false)
	, connected_(false)
	, ping_context(nullptr) {
		mysql_init(&mysql_);
		uv_timer_init(flame::loop, &ping_);
		ping_.data = this;
	}
	void client_implement::start() {
		uv_timer_stop(&ping_);
		ping_context = new client_request_t {
			coroutine::current, this
		};
		ping_context->req.data = ping_context;
		uv_timer_start(&ping_, ping_cb, client_->ping_interval, 0);
	}
	void client_implement::ping_cb(uv_timer_t* handle) {
		client_implement* self = reinterpret_cast<client_implement*>(handle->data);
		self->worker_.queue_work(&self->ping_context->req, client_implement::ping_wk, ping_cb);
	}
	void client_implement::ping_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(!ctx->self->connected_) { // 若还未连接成功
			return;
		}
		if(mysql_query(&ctx->self->mysql_, "SELECT 1") == 0) {
			MYSQL_RES* rs = mysql_store_result(&ctx->self->mysql_);
			if(rs != nullptr) {
				mysql_free_result(rs);
			}
		}
	}
	void client_implement::ping_cb(uv_work_t* req, int status) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		uv_timer_start(&ctx->self->ping_, ping_cb, ctx->self->client_->ping_interval, 0);
	}
	void client_implement::destroy() {
		uv_timer_stop(&ping_);
		uv_close((uv_handle_t*)&ping_, destroy_cb);
	}
	void client_implement::destroy_cb(uv_handle_t* handle) {
		client_implement* self = reinterpret_cast<client_implement*>(handle->data);
		self->worker_.close_work(self, client_implement::destroy_wk, client_implement::destroy_cb);
	}
	void client_implement::connect_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->self->connected_) { // 重新连接，关闭现有
			ctx->self->connected_ = false;
			mysql_close(&ctx->self->mysql_);
			// 重新创建
			mysql_init(&ctx->self->mysql_);
		}
		if(!ctx->self->client_->url_) {
			ctx->rv = php::string("failed to parse mysql connection uri", 36);
			return;
		}
		// 使用 URL 进行连接
		if(mysql_real_connect(&ctx->self->mysql_, ctx->self->client_->url_->host,
			ctx->self->client_->url_->user, ctx->self->client_->url_->pass,
			ctx->self->client_->url_->path+1, ctx->self->client_->url_->port,
			nullptr, 0) == nullptr) {
			// 连接失败，错误在主线程获取生成
			ctx->rv.ptr(&ctx->self->mysql_);
			return;
		}
		// 连接成功
		ctx->self->connected_ = true;
		mysql_autocommit(&ctx->self->mysql_, true);
		ctx->rv = php::BOOL_YES;
	}
	void client_implement::query_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(!ctx->self->connected_) { // 若还未连接成功
			ctx->rv = nullptr;
			return;
		}
		if(ctx->self->debug_) { // 调试输出实际执行的 SQL
			std::fprintf(stderr, "[%s] (flame\\db\\mysql): %.*s\n", time::datetime(time::now()), ctx->sql.length(), ctx->sql.c_str());
		}
		if(mysql_query(&ctx->self->mysql_, ctx->sql.c_str()) != 0) {
			// 查询失败
			ctx->rv.ptr(&ctx->self->mysql_);
			return;
		}
		// 由于在协程运行期间可能存在多次 QUERY 故
		// 这里不能使用 mysql_use_result 而目前用法，
		// 直接使用 store_result 将所有结果集取回，会一定程度上加重内存占用
		MYSQL_RES* rs = mysql_store_result(&ctx->self->mysql_);
		// INSERT / UPDATE / DELETE “更新”型 SQL 执行结果
		if(rs == nullptr && mysql_field_count(&ctx->self->mysql_) == 0) {
			ctx->self->client_->prop("affected_rows") = (int64_t) mysql_affected_rows(&ctx->self->mysql_);
			ctx->self->client_->prop("insert_id")     = (int64_t) mysql_insert_id(&ctx->self->mysql_);
			ctx->rv = php::BOOL_YES;
			return;
		}
		// SELECT 查询型 SQL 执行失败
		if(rs == nullptr) {
			ctx->rv.ptr(&ctx->self->mysql_);
			return;
		}
		// 生成结果集对象须在主线程进行
		ctx->rv.ptr(rs);
	}
	void client_implement::one_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(!ctx->self->connected_) { // 若还未连接成功
			ctx->rv = nullptr;
			return;
		}
		if(ctx->self->debug_) { // 调试输出实际执行的 SQL
			std::fprintf(stderr, "[%s] (flame\\db\\mysql): %.*s\n", time::datetime(time::now()), ctx->sql.length(), ctx->sql.c_str());
		}
		if(mysql_query(&ctx->self->mysql_, ctx->sql.c_str()) != 0) {
			// 查询失败
			ctx->rv.ptr(&ctx->self->mysql_);
			return;
		}
		// 由于在协程运行期间可能存在多次 QUERY 故
		// 这里不能使用 mysql_use_result 而目前用法，
		// 直接使用 store_result 将所有结果集取回，会一定程度上加重内存占用
		MYSQL_RES* rs = mysql_store_result(&ctx->self->mysql_);
		// INSERT / UPDATE / DELETE “更新”型 SQL 执行结果
		if(rs == nullptr) {
			ctx->rv.ptr(&ctx->self->mysql_);
			return;
		}
		// 直接取出结果
		sql_fetch_row(rs, ctx->rv, MYSQL_FETCH_ASSOC);
		mysql_free_result(rs);
	}
	void client_implement::found_rows_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(!ctx->self->connected_) { // 若还未连接成功
			ctx->rv = nullptr;
			return;
		}
		if(ctx->self->debug_) { // 调试输出实际执行的 SQL
			std::fprintf(stderr, "[%s] (flame\\db\\mysql): %.*s\n", time::datetime(time::now()), ctx->sql.length(), ctx->sql.c_str());
		}
		if(mysql_query(&ctx->self->mysql_, ctx->sql.c_str()) != 0) {
			// 查询失败
			ctx->rv.ptr(&ctx->self->mysql_);
			return;
		}
		// 由于在协程运行期间可能存在多次 QUERY 故
		// 这里不能使用 mysql_use_result 而目前用法，
		// 直接使用 store_result 将所有结果集取回，会一定程度上加重内存占用
		MYSQL_RES* rs = mysql_store_result(&ctx->self->mysql_);
		// INSERT / UPDATE / DELETE “更新”型 SQL 执行结果
		if(rs == nullptr) {
			ctx->rv.ptr(&ctx->self->mysql_);
			return;
		}
		// 直接取出结果
		php::array rv;
		sql_fetch_row(rs, rv, MYSQL_FETCH_NUM);
		mysql_free_result(rs);
		ctx->rv = rv[0].to_long();
	}
	void client_implement::destroy_wk(uv_work_t* req) {
		client_implement* self = reinterpret_cast<client_implement*>(req->data);
		mysql_close(&self->mysql_);
		if(self->ping_context != nullptr) {
			delete self->ping_context;
			self->ping_context = nullptr;
		}
	}
	void client_implement::destroy_cb(uv_work_t* req, int status) {
		client_implement* self = reinterpret_cast<client_implement*>(req->data);
		delete self;
	}

}
}
}
