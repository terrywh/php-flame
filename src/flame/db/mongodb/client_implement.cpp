#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "client_implement.h"

namespace flame {
namespace db {
namespace mongodb {
	client_implement::client_implement(client* cli) 
	: client_(cli)
	, cli_(nullptr)
	, uri_(nullptr)
	{
		
	}
	void client_implement::connect_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		// 已存在的连接关闭
		if(ctx->self->cli_) mongoc_client_destroy(ctx->self->cli_);
		if(ctx->name.is_string()) {
			ctx->self->uri_ = mongoc_uri_new(ctx->name.c_str());
		}
		if(!ctx->self->uri_) {
			ctx->rv = php::string("failed to parse mongodb connection uri", 38);
		}
		ctx->self->cli_ = mongoc_client_new_from_uri(ctx->self->uri_);
		if(ctx->self->cli_) {
			mongoc_read_prefs_t* prefs = mongoc_read_prefs_new(MONGOC_READ_SECONDARY_PREFERRED);
			mongoc_client_set_read_prefs(ctx->self->cli_, prefs);
			mongoc_read_prefs_destroy(prefs);
			ctx->rv = php::BOOL_NO;
		}else{
			ctx->rv = php::string("failed to connect to mongodb server", 35);
		}
	}
	void client_implement::collection_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(!ctx->self->cli_) {
			ctx->rv = nullptr;
			return;
		}
		mongoc_collection_t* col = mongoc_client_get_collection(
			ctx->self->cli_,
			mongoc_uri_get_database(ctx->self->uri_),
			ctx->name.c_str());
		
		// collection 创建过程移动到主线程进行
		ctx->rv.ptr(col);
	}
	void client_implement::destroy_wk(uv_work_t* req) {
		client_implement* self = reinterpret_cast<client_implement*>(req->data);
		mongoc_client_destroy(self->cli_);
	}
	void client_implement::destroy_cb(uv_work_t* req, int status) {
		delete reinterpret_cast<client_implement*>(req->data);
	}
}
}
}
