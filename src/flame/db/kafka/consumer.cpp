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
		consumer_request_t* ctx = new consumer_request_t {
			nullptr, impl, nullptr
		};
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req, consumer_implement::close_wk, default_cb);
		return nullptr;
	}
	php::value consumer::consume(php::parameters& params) {
		consumer_request_t* ctx = new consumer_request_t {
			coroutine::current, impl, this
		};
		ctx->req.data = ctx;
		// TODO 暂时屏蔽回调方式的消费
		// （需要提供从 worker -> master 的单独通知，调用回调函数）
		// if(params.length() > 0 && params[0].is_callable()) {
		// 	ctx_ = ctx;
		// 	impl->worker_.queue_work(&ctx->req, consumer_implement::consume2_wk, consume_cb);
		// }else{
			impl->worker_.queue_work(&ctx->req, consumer_implement::consume_wk, consume_cb);
		//}
		return flame::async();
	}
	void consumer::consume_cb(uv_work_t* handle, int status) {
		consumer_request_t* ctx = reinterpret_cast<consumer_request_t*>(handle->data);
		if(ctx->rv.is_pointer()) {
			rd_kafka_message_t* msg = ctx->rv.ptr<rd_kafka_message_t>();
			
			php::object obj = php::object::create<message>();
			message*    cpp = obj.native<message>();
			cpp->init(msg, ctx->self->consumer_);
			ctx->rv = std::move(obj);
		}
		ctx->co->next(ctx->rv);
		delete ctx;
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
			coroutine::current, impl, this, params[0]
		};
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req, consumer_implement::commit_wk, default_cb);
		return flame::async();
	}
}
}
}
