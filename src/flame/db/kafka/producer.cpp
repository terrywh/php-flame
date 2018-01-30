#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "producer_implement.h"
#include "producer.h"


namespace flame {
namespace db {
namespace kafka {
	php::value producer::__construct(php::parameters& params) {
		impl = new producer_implement(this, params);
		return nullptr;
	}
	php::value producer::__destruct(php::parameters& params) {
		impl->worker_.close_work(impl, producer_implement::destroy_wk, producer_implement::destroy_cb);
		return nullptr;
	}
	php::value producer::produce(php::parameters& params) {
		producer_request_t* ctx = new producer_request_t {
			coroutine::current, impl, nullptr, params[0].to_string()
		};
		ctx->req.data = ctx;
		if(params.length() > 1) {
			ctx->key = params[1].to_string();
		}
		impl->worker_.queue_work(&ctx->req, producer_implement::produce_wk, default_cb);
		return flame::async(this);
	}
	void producer::default_cb(uv_work_t* handle, int status) {
		producer_request_t* ctx = reinterpret_cast<producer_request_t*>(handle->data);
		if(ctx->co) ctx->co->next(ctx->rv);
		delete ctx;
	}
	php::value producer::flush(php::parameters& params) {
		producer_request_t* ctx = new producer_request_t {
			coroutine::current, impl
		};
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req, producer_implement::flush_wk, default_cb);
		return flame::async(this);
	}
}
}
}
