#include "../../time/time.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "client.h"
#include "client_implement.h"

namespace flame {
namespace db {
namespace mysql {
	client_implement::client_implement(client* cli)
	: client_(cli)
	, mysql_(mysqlnd_init(MYSQLND_CLIENT_KNOWS_RSET_COPY_DATA, true))
	, debug_(false)
	, connected_(false)
	, ping_context(nullptr) {
		uv_timer_init(flame::loop, &ping_);
		ping_.data = this;
	}
	void client_implement::connect_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->self->connected_) { // 重新连接，关闭现有
			ctx->self->connected_ = false;
			mysqlnd_close(ctx->self->mysql_, MYSQLND_CLOSE_EXPLICIT);
			// 重新创建
			ctx->self->mysql_ = mysqlnd_init(MYSQLND_CLIENT_KNOWS_RSET_COPY_DATA, true);
		}
		if(!ctx->self->client_->url_) {
			ctx->rv = php::string("failed to parse mysql connection uri", 36);
			return;
		}
		// 使用 URL 进行连接
		if(mysqlnd_connect(
			ctx->self->mysql_, ctx->self->client_->url_->host,
			ctx->self->client_->url_->user, ctx->self->client_->url_->pass,
			std::strlen(ctx->self->client_->url_->pass), ctx->self->client_->url_->path+1,
			std::strlen(ctx->self->client_->url_->path) - 1, ctx->self->client_->url_->port,
			nullptr, 0,
			MYSQLND_CLIENT_KNOWS_RSET_COPY_DATA) == nullptr) {
			// 连接失败，错误在主线程获取生成
			ctx->rv.ptr(ctx->self->mysql_);
			return;
		}
		// 连接成功
		ctx->self->connected_ = true;
		mysqlnd_autocommit(ctx->self->mysql_, true);
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
		if(0 != mysqlnd_query(ctx->self->mysql_, ctx->sql.c_str(), ctx->sql.length())) {
			// 查询失败
			ctx->rv.ptr(ctx->self->mysql_);
			return;
		}
		// 由于在协程运行期间可能存在多次 QUERY 故
		// 这里不能使用 mysqlnd_use_result 而目前用法，
		// 直接使用 store_result 将所有结果集取回，会一定程度上加重内存占用
		MYSQLND_RES* rs = mysqlnd_store_result(ctx->self->mysql_);
		// INSERT / UPDATE / DELETE “更新”型 SQL 执行结果
		if(rs == nullptr && mysqlnd_field_count(ctx->self->mysql_) == 0) {
			ctx->self->client_->prop("affected_rows") = mysqlnd_affected_rows(ctx->self->mysql_);
			ctx->self->client_->prop("insert_id")     = mysqlnd_insert_id(ctx->self->mysql_);
			ctx->rv = php::BOOL_YES;
			return;
		}
		// SELECT 查询型 SQL 执行失败
		if(rs == nullptr) {
			ctx->rv.ptr(ctx->self->mysql_);
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
		if(FAIL == mysqlnd_query(ctx->self->mysql_, ctx->sql.c_str(), ctx->sql.length())) {
			// 查询失败
			ctx->rv.ptr(ctx->self->mysql_);
			return;
		}
		// 由于在协程运行期间可能存在多次 QUERY 故
		// 这里不能使用 mysqlnd_use_result 而目前用法，
		// 直接使用 store_result 将所有结果集取回，会一定程度上加重内存占用
		MYSQLND_RES* rs = mysqlnd_store_result(ctx->self->mysql_);
		// INSERT / UPDATE / DELETE “更新”型 SQL 执行结果
		if(rs == nullptr) {
			ctx->rv.ptr(ctx->self->mysql_);
			return;
		}
		// 直接取出结果
		zval rv;
		mysqlnd_fetch_into(rs, MYSQLND_FETCH_ASSOC, &rv, MYSQLND_MYSQLI);
		ctx->rv = php::value(rv);
		mysqlnd_free_result(rs, false);
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
		if(0 != mysqlnd_query(ctx->self->mysql_, ctx->sql.c_str(), ctx->sql.length())) {
			// 查询失败
			ctx->rv.ptr(ctx->self->mysql_);
			return;
		}
		// 由于在协程运行期间可能存在多次 QUERY 故
		// 这里不能使用 mysqlnd_use_result 而目前用法，
		// 直接使用 store_result 将所有结果集取回，会一定程度上加重内存占用
		MYSQLND_RES* rs = mysqlnd_store_result(ctx->self->mysql_);
		// INSERT / UPDATE / DELETE “更新”型 SQL 执行结果
		if(rs == nullptr) {
			ctx->rv.ptr(ctx->self->mysql_);
			return;
		}
		// 直接取出结果
		zval rv;
		mysqlnd_fetch_into(rs, MYSQLND_FETCH_NUM, &rv, MYSQLND_MYSQLI);
		ctx->rv = php::value(rv);
		mysqlnd_free_result(rs, false);
		php::array& row = ctx->rv;
		ctx->rv = row[0].to_long();
	}
	void client_implement::ping_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(!ctx->self->connected_) { // 若还未连接成功
			return;
		}
		if(mysqlnd_query(ctx->self->mysql_, "SELECT 1", 8) != 0) return;
		MYSQLND_RES* rs = mysqlnd_store_result(ctx->self->mysql_);
		if(rs == nullptr) return;
		mysqlnd_free_result(rs, false);
	}
	void client_implement::destroy_wk(uv_work_t* req) {
		client_implement* self = reinterpret_cast<client_implement*>(req->data);
		if(self->mysql_ != nullptr) {
			mysqlnd_close(self->mysql_, MYSQLND_CLOSE_EXPLICIT);
			self->mysql_ = nullptr;
		}
		if(self->ping_context != nullptr) {
			delete self->ping_context;
			self->ping_context = nullptr;
		}
	}
	void client_implement::destroy_cb(uv_work_t* req, int status) {
		delete reinterpret_cast<client_implement*>(req->data);
	}
	void client_implement::begin_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(!ctx->self->connected_) { // 若还未连接成功
			ctx->rv = nullptr;
			return;
		}
		if(mysqlnd_begin_transaction(ctx->self->mysql_, TRANS_START_NO_OPT, nullptr) == FAIL) {
			ctx->rv.ptr(ctx->self->mysql_);
		}else{
			ctx->rv = php::BOOL_YES;
		}
	}
	void client_implement::commit_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(!ctx->self->connected_) { // 若还未连接成功
			ctx->rv = nullptr;
			return;
		}
		if(mysqlnd_commit(ctx->self->mysql_, TRANS_START_NO_OPT, nullptr) == FAIL) {
			ctx->rv.ptr(ctx->self->mysql_);
		}else{
			ctx->rv = php::BOOL_YES;
		}
	}
	void client_implement::rollback_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(!ctx->self->connected_) { // 若还未连接成功
			ctx->rv = nullptr;
			return;
		}
		if(mysqlnd_rollback(ctx->self->mysql_, TRANS_START_NO_OPT, nullptr) == FAIL) {
			ctx->rv.ptr(ctx->self->mysql_);
		}else{
			ctx->rv = php::BOOL_YES;
		}
	}

}
}
}
