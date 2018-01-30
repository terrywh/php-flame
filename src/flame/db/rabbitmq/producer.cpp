#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "table.h"
#include "client_implement.h"
#include "producer.h"

namespace flame {
namespace db {
namespace rabbitmq {
	producer::producer()
	: opt_exchange(amqp_empty_bytes)
	, opt_mandatory(0)
	, opt_immediate(0) {}
	php::value producer::__construct(php::parameters& params) {
		impl = new client_implement();
		impl->producer_ = this;
		auto url_ = impl->parse_url(params[0]);
		impl->connect(url_);

		if(params.length() > 1) {
			if(params[1].is_array()) {
				php::array&  options = params[1];
				opt_mandatory = !options.at("mandatory",9).is_empty();
				opt_immediate = !options.at("immediate",9).is_empty();
			}else if(params[1].is_string()) {
				php_exchange       = params[1];
				opt_exchange.len   = php_exchange.length();
				opt_exchange.bytes = php_exchange.data();
			}
		}
		if(params.length() > 2 && params[2].is_string()) {
			php_exchange       = params[2];
			opt_exchange.len   = php_exchange.length();
			opt_exchange.bytes = php_exchange.data();
		}
		return nullptr;
	}
	php::value producer::__destruct(php::parameters& params) {
		impl->worker_.close_work(impl, client_implement::destroy_wk, client_implement::destroy_cb);
		return nullptr;
	}
	php::value producer::produce(php::parameters& params) {
		if(params.length() > 0 && !params[0].is_string()) {
			throw php::exception("produce failed: message body must be a string");
		}
		client_request_t* ctx = new client_request_t {
			coroutine::current, impl, params[0]
		};
		ctx->req.data = ctx;
		if(params.length() > 1) {
			if(!params[1].is_string()) {
				throw php::exception("produce failed: routing key must be a string");
			}
			ctx->key = params[1];
		}
		if(params.length() > 2) {
			if(!params[2].is_array()) {
				throw php::exception("produce failed: options must be an array");
			}
			ctx->rv = params[2];
		}
		impl->worker_.queue_work(&ctx->req, client_implement::produce_wk, client_implement::error_cb);
		return flame::async(this);
	}
	php::value producer::flush(php::parameters& params) {
		client_request_t* ctx = new client_request_t {
			coroutine::current, impl
		};
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req, client_implement::flush_wk, client_implement::error_cb);
		return flame::async(this);
	}
}
}
}