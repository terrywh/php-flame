#include "../../coroutine.h"
#include "client.h"
#include "collection.h"

namespace flame {
namespace db {
namespace mongodb {
	client::client()
	: client_(nullptr)
	, uri_(nullptr) {}
	typedef struct client_request_t {
		coroutine*        co;
		mongoc_client_t* cli;
		php::value        rv;
		php::string     name;
		uv_work_t        req;
	} client_request_t;
	void client::default_cb(uv_work_t* req, int status) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		ctx->co->next(ctx->rv);
		delete ctx;
	}
	void client::connect_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		client* self = reinterpret_cast<php::object&>(ctx->rv).native<client>();
		if(ctx->cli) mongoc_client_destroy(ctx->cli);
		ctx->cli = mongoc_client_new_from_uri(self->uri_);
		if(ctx->cli) {
			self->client_ = ctx->cli;
			ctx->rv = (bool)true;
		}else{
			self->client_ = nullptr;
			ctx->rv = php::make_exception("failed to connect to mongodb");
		}
	}
	php::value client::connect(php::parameters& params) {
		if(params.length() > 0 && params[0].is_string()) {
			php::string& uri = params[0];
			uri_ = mongoc_uri_new(uri.c_str());
		}
		if(!uri_) {
			throw php::exception("failed to construct mongodb client: failed to parse URI");
		}
		connect_();
		return flame::async();
	}
	void client::connect_() {
		client_request_t* ctx = new client_request_t {
			coroutine::current, client_, this
		};
		ctx->req.data = ctx;
		worker_.queue_work(&ctx->req, connect_wk, default_cb);
	}
	php::value client::__destruct(php::parameters& params) {
		if(client_) {
			close(params);
		}
		return nullptr;
	}
	void client::close_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		client* self = reinterpret_cast<php::object&>(ctx->rv).native<client>();
		mongoc_client_destroy(ctx->cli);
		ctx->rv = (bool)true;
	}
	php::value client::close(php::parameters& params) {
		if(client_) {
			client_request_t* ctx = new client_request_t {
				coroutine::current, client_, this
			};
			client_ = nullptr;
			ctx->req.data = ctx;
			worker_.queue_work(&ctx->req, close_wk, default_cb);
			return flame::async();
		}
		return nullptr;
	}
	void client::collection_wk(uv_work_t* req) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(!ctx->cli) {
			ctx->rv = php::make_exception("mongodb not connected");
			return;
		}
		client* self = reinterpret_cast<php::object&>(ctx->rv).native<client>();

		mongoc_collection_t* col = mongoc_client_get_collection(ctx->cli,
			mongoc_uri_get_database(self->uri_), ctx->name.c_str());
		php::object          obj = php::object::create<mongodb::collection>();
		mongodb::collection* cpp = obj.native<mongodb::collection>();
		ctx->rv = std::move(obj);
		cpp->init(self, self->client_, col);
	}
	php::value client::collection(php::parameters& params) {
		client_request_t* ctx = new client_request_t {
			coroutine::current, client_, this, params[0]
		};
		ctx->req.data = ctx;
		worker_.queue_work(&ctx->req, collection_wk, default_cb);
		return flame::async();
	}
}
}
}
