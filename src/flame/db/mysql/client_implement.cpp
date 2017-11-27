#include "../../time/time.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "client_implement.h"
#include "result_set.h"

namespace flame {
namespace db {
namespace mysql {
	client_implement::client_implement(std::shared_ptr<thread_worker> worker, client* cli)
	: worker_(worker)
	, mysql_(nullptr)
	, debug_(false)
	, ping_interval(60 * 1000)
	, url_(nullptr) {
		uv_timer_init(&worker_->loop, &ping_);
		ping_.data = this;
	}
	void client_implement::connect_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->self->mysql_ != nullptr) { // 重新连接，关闭现有
			mysqlnd_close(ctx->self->mysql_, MYSQLND_CLOSE_EXPLICIT);
		}
		// 解析 URL 参数（重新连接的情况可能没有指定新的字符串）
		if(ctx->sql.is_string()) {
			ctx->self->url_ = php::parse_url(ctx->sql.c_str(), ctx->sql.length());
			if(strncasecmp(ctx->self->url_->scheme, "mysql", 5) != 0 || std::strlen(ctx->self->url_->path) < 1) {
				ctx->rv = php::make_exception("failed to parse mysql connection uri");
				return;
			}
			if(!ctx->self->url_->port) { // 默认端口
				ctx->self->url_->port = 3306;
			}
		}else if(!ctx->self->url_) {
			ctx->rv = php::make_exception("failed to parse mysql connection uri");
		}
		// 使用 URL 进行连接
		ctx->self->mysql_ = mysqlnd_init(MYSQLND_CLIENT_KNOWS_RSET_COPY_DATA, true);
		ctx->self->mysql_ = mysqlnd_connect(
			ctx->self->mysql_,
			ctx->self->url_->host,
			ctx->self->url_->user,
			ctx->self->url_->pass,
			std::strlen(ctx->self->url_->pass),
			ctx->self->url_->path+1,
			std::strlen(ctx->self->url_->path) - 1,
			ctx->self->url_->port,
			nullptr,
			0,
			MYSQLND_CLIENT_KNOWS_RSET_COPY_DATA);
		if(ctx->self->mysql_ == nullptr) { // 连接失败
			ctx->rv = php::make_exception(mysqlnd_error(ctx->self->mysql_), mysqlnd_errno(ctx->self->mysql_));
			mysqlnd_close(ctx->self->mysql_, MYSQLND_CLOSE_DISCONNECTED);
			return;
		}
		// 连接成功
		mysqlnd_autocommit(ctx->self->mysql_, true);
		ctx->rv = bool(true);
		uv_timer_start(&ctx->self->ping_, ping_cb, ctx->self->ping_interval, 0);
	}
	void client_implement::query_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->self->mysql_ == nullptr) { // 若还未连接成功
			ctx->rv = nullptr;
			return;
		}
		if(ctx->self->debug_) { // 调试输出实际执行的 SQL
			std::printf("[%s] (flame\\db\\mysql): %.*s\n", time::datetime(time::now()), ctx->sql.length(), ctx->sql.c_str());
		}
		if(0 != mysqlnd_query(ctx->self->mysql_, ctx->sql.c_str(), ctx->sql.length())) {
			// 查询失败
			ctx->rv = php::make_exception(
				mysqlnd_error(ctx->self->mysql_),
				mysqlnd_errno(ctx->self->mysql_));
			return;
		}
		// 由于在协程运行期间可能存在多次 QUERY 故
		// 这里不能使用 mysqlnd_use_result 而目前用法，
		// 直接使用 store_result 将所有结果集取回，会一定程度上加重内存占用
		MYSQLND_RES* rs = mysqlnd_store_result(ctx->self->mysql_);
		// INSERT / UPDATE / DELETE “更新”型 SQL 执行结果
		if(rs == nullptr && mysqlnd_field_count(ctx->self->mysql_) == 0) {
			reinterpret_cast<php::object&>(ctx->rv).prop("affected_rows") = mysqlnd_affected_rows(ctx->self->mysql_);
			reinterpret_cast<php::object&>(ctx->rv).prop("insert_id") = mysqlnd_insert_id(ctx->self->mysql_);
			ctx->rv = (bool)true;
			return;
		}
		// SELECT 查询型 SQL 执行失败
		if(rs == nullptr) {
			ctx->rv = php::make_exception(mysqlnd_error(ctx->self->mysql_), mysqlnd_errno(ctx->self->mysql_));
			return;
		}
		// 生成结果集对象
		php::object obj = php::object::create<result_set>();
		result_set* cpp = obj.native<result_set>();
		ctx->rv = std::move(obj);
		cpp->init(ctx->self->worker_, ctx->self->client_, rs);
	}
	void client_implement::one_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->self->mysql_ == nullptr) { // 若还未连接成功
			ctx->rv = nullptr;
			return;
		}
		if(ctx->self->debug_) { // 调试输出实际执行的 SQL
			std::printf("[%s] (flame\\db\\mysql): %.*s\n", time::datetime(time::now()), ctx->sql.length(), ctx->sql.c_str());
		}
		if(FAIL == mysqlnd_query(ctx->self->mysql_, ctx->sql.c_str(), ctx->sql.length())) {
			// 查询失败
			ctx->rv = php::make_exception(
				mysqlnd_error(ctx->self->mysql_),
				mysqlnd_errno(ctx->self->mysql_));
			return;
		}
		// 由于在协程运行期间可能存在多次 QUERY 故
		// 这里不能使用 mysqlnd_use_result 而目前用法，
		// 直接使用 store_result 将所有结果集取回，会一定程度上加重内存占用
		MYSQLND_RES* rs = mysqlnd_store_result(ctx->self->mysql_);
		// INSERT / UPDATE / DELETE “更新”型 SQL 执行结果
		if(rs == nullptr) {
			ctx->rv = php::make_exception(mysqlnd_error(ctx->self->mysql_), mysqlnd_errno(ctx->self->mysql_));
			return;
		}
		// 直接取出结果
		mysqlnd_fetch_into(rs, MYSQLND_FETCH_ASSOC, (zval*)&ctx->rv, MYSQLND_MYSQLI);
		mysqlnd_free_result(rs, false);
	}
	void client_implement::found_rows_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->self->mysql_ == nullptr) { // 若还未连接成功
			ctx->rv = nullptr;
			return;
		}
		if(ctx->self->debug_) { // 调试输出实际执行的 SQL
			std::printf("[%s] (flame\\db\\mysql): %.*s\n", time::datetime(time::now()), ctx->sql.length(), ctx->sql.c_str());
		}
		if(0 != mysqlnd_query(ctx->self->mysql_, ctx->sql.c_str(), ctx->sql.length())) {
			// 查询失败
			ctx->rv = php::make_exception(
				mysqlnd_error(ctx->self->mysql_),
				mysqlnd_errno(ctx->self->mysql_));
			return;
		}
		// 由于在协程运行期间可能存在多次 QUERY 故
		// 这里不能使用 mysqlnd_use_result 而目前用法，
		// 直接使用 store_result 将所有结果集取回，会一定程度上加重内存占用
		MYSQLND_RES* rs = mysqlnd_store_result(ctx->self->mysql_);
		// INSERT / UPDATE / DELETE “更新”型 SQL 执行结果
		if(rs == nullptr) {
			ctx->rv = php::make_exception(mysqlnd_error(ctx->self->mysql_), mysqlnd_errno(ctx->self->mysql_));
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
		if(self->mysql_ == nullptr) return;
		
		if(FAIL == mysqlnd_ping(self->mysql_)) {
			return;
		}
		uv_timer_start(&self->ping_, ping_cb, self->ping_interval, 0);
	}
	void client_implement::close_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		uv_close((uv_handle_t*)&ctx->self->ping_, close_cb);
	}
	void client_implement::close_cb(uv_handle_t* handle) {
		client_implement* self = reinterpret_cast<client_implement*>(handle->data);
		if(self->mysql_ != nullptr) {
			mysqlnd_close(self->mysql_, MYSQLND_CLOSE_EXPLICIT);
			// self->mysql_ = nullptr;
		}
		// 由于工作线程可能与 result_set 对象共享，故这里不做操作
		delete self;
	}
}
}
}
