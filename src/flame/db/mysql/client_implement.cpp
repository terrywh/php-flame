#include "../../time/time.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "client.h"
#include "client_implement.h"

namespace flame {
namespace db {
namespace mysql {
	client_implement::client_implement(std::shared_ptr<thread_worker> worker, client* cli)
	: worker_(worker)
	, client_(cli)
	, mysql_(mysqlnd_init(MYSQLND_CLIENT_KNOWS_RSET_COPY_DATA, true))
	, debug_(false)
	, ping_interval(60 * 1000)
	, url_(nullptr)
	, connected_(false) {
		uv_timer_init(&worker_->loop, &ping_);
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
		// 解析 URL 参数（重新连接的情况可能没有指定新的字符串）
		if(ctx->sql.is_string()) {
			ctx->self->url_ = php::parse_url(ctx->sql.c_str(), ctx->sql.length());
			if(strncasecmp(ctx->self->url_->scheme, "mysql", 5) != 0 || std::strlen(ctx->self->url_->path) < 1) {
				ctx->rv = php::string("failed to parse mysql connection uri", 36);
				return;
			}
			if(!ctx->self->url_->port) { // 默认端口
				ctx->self->url_->port = 3306;
			}
		}else if(!ctx->self->url_) {
			ctx->rv = php::string("failed to parse mysql connection uri", 36);
		}
		// 使用 URL 进行连接
		if(mysqlnd_connect(
			ctx->self->mysql_, ctx->self->url_->host,
			ctx->self->url_->user, ctx->self->url_->pass,
			std::strlen(ctx->self->url_->pass), ctx->self->url_->path+1,
			std::strlen(ctx->self->url_->path) - 1, ctx->self->url_->port,
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
		uv_timer_start(&ctx->self->ping_, ping_cb, ctx->self->ping_interval, 0);
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
			ctx->rv = php::BOOL_NO;
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
		mysqlnd_fetch_into(rs, MYSQLND_FETCH_ASSOC, (zval*)&ctx->rv, MYSQLND_MYSQLI);
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
		mysqlnd_fetch_into(rs, MYSQLND_FETCH_NUM, (zval*)&ctx->rv, MYSQLND_MYSQLI);
		mysqlnd_free_result(rs, false);
		php::array& row = ctx->rv;
		ctx->rv = row[0].to_long();
	}
	void client_implement::ping_cb(uv_timer_t* req) {
		client_implement* self = reinterpret_cast<client_implement*>(req->data);
		if(!self->connected_) { // 若还未连接成功
			return;
		}
		
		if(0 != mysqlnd_query(self->mysql_, "SELECT 1", 8)) return;
		MYSQLND_RES* rs = mysqlnd_store_result(self->mysql_);
		if(rs == nullptr) return;
		mysqlnd_free_result(rs, false);
		uv_timer_start(&self->ping_, ping_cb, self->ping_interval, 0);
	}
	void client_implement::close_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->self->mysql_ != nullptr) {
			mysqlnd_close(ctx->self->mysql_, MYSQLND_CLOSE_EXPLICIT);
			ctx->self->mysql_ = nullptr;
		}
		uv_close((uv_handle_t*)&ctx->self->ping_, close_cb);
	}
	void client_implement::close_cb(uv_handle_t* handle) {
		client_implement* self = reinterpret_cast<client_implement*>(handle->data);
		// 由于工作线程可能与 result_set 对象共享，故这里不做操作
		delete self;
	}
}
}
}
