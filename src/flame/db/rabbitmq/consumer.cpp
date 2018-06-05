#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "table.h"
#include "client_implement.h"
#include "message.h"
#include "consumer.h"

namespace flame {
namespace db {
namespace rabbitmq {
	consumer::consumer()
	: opt_no_local(0)
	, opt_no_ack(0) 
	, opt_exclusive(0)
	, opt_prefetch(1)
	, opt_arguments(amqp_empty_table) {

	}
	php::value consumer::__construct(php::parameters& params) {
		if(params.length() < 3 || !params[0].is_string() || !params[1].is_array()
			|| params[2].is_array() && !params[2].is_string()) {
			throw php::exception("failed to create rabbitmq consumer, wrong parameters");
		}
		impl = new client_implement(false);
		impl->consumer_ = this;
		auto url_ = impl->parse_url(params[0]);
		impl->connect(url_);
		php::array& options = static_cast<php::array&>(params[1]);
			
		opt_no_local    = !options.at("no_local",8).is_empty();
		opt_no_ack      = !options.at("no_ack",6).is_empty();
		opt_exclusive   = !options.at("exclusive",9).is_empty();
		if(!options.has("prefetch", 8)) {
			opt_prefetch = 1;
		}else{
			php::array_item_assoc prefetch = options.at("prefetch");
		    opt_prefetch = static_cast<int>(prefetch);
		}
		if(params[2].is_array()) {
			php::array& qs = static_cast<php::array&>(params[2]);
			for(auto i=qs.begin(); i!=qs.end(); ++i) {
				impl->subscribe(i->second.to_string(), opt_prefetch);
			}
		}else if(params[2].is_string()) {
			impl->subscribe(params[2], opt_prefetch);
		}
		return nullptr;
	}
	php::value consumer::__destruct(php::parameters& params) {
		impl->worker_.close_work(impl, client_implement::destroy_wk, client_implement::destroy_cb);
		return nullptr;
	}
	php::value consumer::consume(php::parameters& params) {
		client_request_t* ctx = new client_request_t {
			coroutine::current, impl, php::object::create<message>()
		};
		if(params.length() > 0 && params[0].is_long()) {
			ctx->key = params[0];
		}else{
			ctx->key = int(0);
		}
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req, client_implement::consume_wk, consume_cb);
		return flame::async(this);
	}
	void consumer::consume_cb(uv_work_t* req, int status) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		int error = static_cast<int>(ctx->rv);
		if(error == AMQP_STATUS_TIMEOUT) {
			ctx->co->next(); // 超时按 null 返回
		}else if(error == 0) {
			message*    msg = static_cast<php::object&>(ctx->msg).native<message>();
			msg->init(ctx->self->consumer_); // 对象数据的生成需要在主线程
			ctx->self->worker_.queue_work(&ctx->req,
				client_implement::destroy_message_wk, client_implement::destroy_message_cb);
			// 复用 ctx 进行 envelope 的销毁
		}else if(error == -2) {
			ctx->co->fail("rabbitmq client already closed");
		}else if(error == -1) {
			ctx->co->fail("rabbitmq server failed");
		}else{
			ctx->co->fail(amqp_error_string2(error), error);
		}
	}
	php::value consumer::confirm(php::parameters& params) {
		php::object& obj = static_cast<php::object&>(params[0]);
		if(!obj.is_object() || !obj.is_instance_of<message>()) {
			throw php::exception("only rabbitmq message can be confirmed");
		}
		client_request_t* ctx = new client_request_t {
			coroutine::current, impl, obj
		};
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req,
			client_implement::confirm_message_wk, client_implement::error_cb);
		return flame::async(this);
	}
	php::value consumer::reject(php::parameters& params) {
		php::object& obj = static_cast<php::object&>(params[0]);
		if(!obj.is_object() || !obj.is_instance_of<message>()) {
			throw php::exception("only rabbitmq message can be rejected");
		}
		client_request_t* ctx = new client_request_t {
			coroutine::current, impl, obj
		};
		if(params.length() > 1 && params[1].is_true()) {
			ctx->key = php::BOOL_TRUE;
		}else{
			ctx->key = php::BOOL_FALSE;
		}
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req,
			client_implement::reject_message_wk, client_implement::error_cb);
		return flame::async(this);
	}
}
}
}
