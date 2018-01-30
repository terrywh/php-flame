#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "client_implement.h"
#include "client.h"
#include "collection.h"

namespace flame {
namespace db {
namespace mongodb {
	php::value client::__construct(php::parameters& params) {
		impl = new client_implement(this);
		return nullptr;
	}
	php::value client::__destruct(php::parameters& params) {
		impl->worker_.close_work(impl, client_implement::destroy_wk, client_implement::destroy_cb);
		return nullptr;
	}
	void client::default_cb(uv_work_t* req, int status) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->co) ctx->co->next(ctx->rv);
		delete ctx;
	}
	php::value client::connect(php::parameters& params) {
		client_request_t* ctx = new client_request_t {
			coroutine::current, impl, nullptr
		};
		ctx->req.data = ctx;
		if(params.length() > 0 && params[0].is_string()) {
			ctx->name = params[0];
		}
		impl->worker_.queue_work(&ctx->req,
			client_implement::connect_wk,
			connect_cb);
		return flame::async(this);
	}
	void client::connect_cb(uv_work_t* req, int status) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->rv.is_string()) {
			php::string& message = ctx->rv;
			ctx->co->fail(message, 0);
		}else{
			ctx->co->next(ctx->rv);
		}
		delete ctx;
	}
	void client::collection_cb(uv_work_t* req, int status) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->rv.is_pointer()) {
			mongoc_collection_t* col = ctx->rv.ptr<mongoc_collection_t>();
			php::object          obj = php::object::create<mongodb::collection>();
			mongodb::collection* cpp = obj.native<mongodb::collection>();
			// ctx->self ===> impl
			cpp->init(&ctx->self->worker_, ctx->self->client_, col);
			ctx->rv = std::move(obj);
		}
		ctx->co->next(ctx->rv);
		delete ctx;
	}
	php::value client::collection(php::parameters& params) {
		client_request_t* ctx = new client_request_t {
			coroutine::current, impl, nullptr, params[0].to_string()
		};
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req, client_implement::collection_wk, collection_cb);
		return flame::async(this);
	}
}
}
}
