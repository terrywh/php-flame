#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "client_implement.h"
#include "client.h"

namespace flame {
namespace db {
namespace mongodb {
	client::client()
	: impl(new client_implement(this)) {
		
	}
	client::~client() {
		client_request_t* ctx = new client_request_t {
			nullptr, impl, nullptr
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req,
			client_implement::close_wk,
			default_cb);
	}
	void client::default_cb(uv_work_t* req, int status) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->co) ctx->co->next(ctx->rv);
		delete ctx;
	}
	php::value client::connect(php::parameters& params) {
		client_request_t* ctx = new client_request_t {
			coroutine::current, impl, this
		};
		ctx->req.data = ctx;
		if(params.length() > 0 && params[0].is_string()) {
			ctx->name = params[0];
		}
		impl->worker_->queue_work(&ctx->req,
			client_implement::connect_wk,
			default_cb);
		return flame::async();
	}
	php::value client::collection(php::parameters& params) {
		client_request_t* ctx = new client_request_t {
			coroutine::current, impl, this, params[0].to_string()
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, client_implement::collection_wk, default_cb);
		return flame::async();
	}
}
}
}
