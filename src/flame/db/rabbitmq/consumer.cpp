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
			coroutine::current, impl
		};
		if(params.length() > 0 && params[0].is_long()) {
			ctx->rv = params[0];
		}else{
			ctx->rv = int(0);
		}
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req, client_implement::consume_wk, consume_cb);
		return flame::async(this);
	}
	void consumer::consume_cb(uv_work_t* req, int status) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->rv.is_long()) {
			client_implement::error_cb(req, status);
		}else if(ctx->rv.is_null()){ // 超时消费情况
			ctx->co->next();
		}else if(ctx->rv.is_pointer()) {
			php::object obj = php::object::create<message>();
			message*    msg = obj.native<message>();
			msg->init(ctx->rv.ptr<amqp_envelope_t>(), ctx->self->consumer_);
			ctx->co->next(std::move(obj));
		}else{
			assert(0);
		}
		delete ctx;
	}
	php::value consumer::confirm(php::parameters& params) {
		php::object& obj = static_cast<php::object&>(params[0]);
		if(!obj.is_object() || !obj.is_instance_of<message>()) {
			throw php::exception("only rabbitmq message can be confirmed");
		}
		client_request_t* ctx = new client_request_t {
			coroutine::current, impl
		};
		ctx->msg = obj;
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req,
			client_implement::confirm_envelope_wk, client_implement::error_cb);
		return flame::async(this);
	}
	php::value consumer::reject(php::parameters& params) {
		php::object& obj = static_cast<php::object&>(params[0]);
		if(!obj.is_object() || !obj.is_instance_of<message>()) {
			throw php::exception("only rabbitmq message can be rejected");
		}
		client_request_t* ctx = new client_request_t {
			coroutine::current, impl
		};
		ctx->msg = obj;
		if(params.length() > 1 && params[1].is_true()) {
			ctx->key = php::BOOL_YES;
		}else{
			ctx->key = php::BOOL_NO;
		}
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req,
			client_implement::reject_envelope_wk, client_implement::error_cb);
		return flame::async(this);
	}
}
}
}
