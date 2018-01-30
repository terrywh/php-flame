#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "consumer_implement.h"
#include "consumer.h"
#include "message.h"

namespace flame {
namespace db {
namespace kafka {
	php::value consumer::__construct(php::parameters& params) {
		impl = new consumer_implement(this, params);
		return nullptr;
	}
	php::value consumer::__destruct(php::parameters& params) {
		impl->worker_.close_work(impl, consumer_implement::destroy_wk, consumer_implement::destroy_cb);
		return nullptr;
	}
	php::value consumer::consume(php::parameters& params) {
		consumer_request_t* ctx = new consumer_request_t {
			coroutine::current, impl, int(0)
		};
		if(params.length() > 0 && params[0].is_long()) {
			ctx->rv = params[0];
		}
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req, consumer_implement::consume_wk, consume_cb);
		return flame::async(this);
	}
	void consumer::consume_cb(uv_work_t* handle, int status) {
		consumer_request_t* ctx = reinterpret_cast<consumer_request_t*>(handle->data);
		if(ctx->msg.is_pointer()) {
			rd_kafka_message_t* msg = ctx->msg.ptr<rd_kafka_message_t>();
			if(msg->err == 0) {
				php::object obj = php::object::create<message>();
				message*    cpp = obj.native<message>();
				cpp->init(msg, ctx->self->consumer_);
				// ctx->rv = std::move(obj);
				ctx->co->next(std::move(obj));
				delete ctx;
			}else{
				ctx->rv = php::object::create_exception(rd_kafka_err2str(msg->err), msg->err);
				ctx->self->worker_.queue_work(&ctx->req, consumer_implement::destroy_msg_wk, default_cb);
			}
		}else{ // 超时的情况
			ctx->co->next(nullptr);
		}
	}
	void consumer::default_cb(uv_work_t* handle, int status) {
		consumer_request_t* ctx = reinterpret_cast<consumer_request_t*>(handle->data);
		if(ctx->co) ctx->co->next(ctx->rv);
		delete ctx;
	}
	php::value consumer::commit(php::parameters& params) {
		php::object& msg = params[0];
		if(!msg.is_instance_of<message>()) {
			throw php::exception("only instanceof flame\\db\\kafka\\message can be commited");
		}
		consumer_request_t* ctx = new consumer_request_t {
			coroutine::current, impl, nullptr, params[0]
		};
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req, consumer_implement::commit_wk, default_cb);
		return flame::async(this);
	}
}
}
}
