#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "client.h"
#include "collection_implement.h"
#include "collection.h"
#include "mongodb.h"
#include "bulk_result.h"
#include "cursor.h"

namespace flame {
namespace db {
namespace mongodb {
	void collection::init(std::shared_ptr<thread_worker> worker, client* cli, mongoc_collection_t* col) {
		impl = new collection_implement(worker, this, col);
		ref_ = cli;
	}
	collection::collection()
	:impl(nullptr) {

	}
	collection::~collection() {
		if(impl) {
			collection_request_t* ctx = new collection_request_t {
				nullptr, impl, nullptr
			};
			ctx->req.data = ctx;
			impl->worker_->queue_work(&ctx->req, collection_implement::close_wk,
				default_cb);
		}
	}
	void collection::default_cb(uv_work_t* w, int status) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		if(ctx->co) ctx->co->next(ctx->rv);
		delete ctx;
	}
	php::value collection::count(php::parameters& params) {
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, this
		};
		ctx->req.data = ctx;
		if(params.length() > 0 && params[0].is_array()) {
			ctx->doc1 = params[0];
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::count_wk, default_cb);
		return flame::async();
	}
	php::value collection::insert_one(php::parameters& params) {
		php::array& doc = params[0];
		if(!doc.is_array() || !doc.is_a_map()) {
			throw php::exception("illegal document, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, this, doc};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, collection_implement::insert_one_wk, default_cb);
		return flame::async();
	}
	php::value collection::insert_many(php::parameters& params) {
		php::array& docs = params[0];
		if(!docs.is_array() || !docs.is_a_list()) {
			throw php::exception("illegal document, document array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, this, docs
		};
		ctx->req.data = ctx;
		if(params.length() > 1 && params[1].is_true()) {
			ctx->flags = true;
		}else{
			ctx->flags = false;
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::insert_many_wk, default_cb);
		return flame::async();
	}
	php::value collection::remove_one(php::parameters& params) {
		php::array& filter = params[0];
		if(!filter.is_array() || !filter.is_a_map()) {
			throw php::exception("illegal selector, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, this, filter
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, collection_implement::remove_one_wk, default_cb);
		return flame::async();
	}
	php::value collection::remove_many(php::parameters& params) {
		php::array& filter = params[0];
		if(!filter.is_array() || !filter.is_a_map()) {
			throw php::exception("illegal selector, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, this, filter
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, collection_implement::remove_many_wk, default_cb);
		return flame::async();
	}
	php::value collection::update_one(php::parameters& params) {
		php::array& filter = params[0];
		php::array& update = params[1];
		if(!filter.is_array() || !filter.is_a_map() || !update.is_array()
			|| !update.is_a_map()) {
			throw php::exception("illegal filter / update, associate array is required");
		}

		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, this, filter, update, MONGOC_UPDATE_NONE
		};
		ctx->req.data = ctx;
		if(params.length() > 2 && params[2].is_true()) {
			ctx->flags |= MONGOC_UPDATE_UPSERT;
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::update_wk, default_cb);
		return flame::async();
	}
	php::value collection::update_many(php::parameters& params) {
		php::array& filter = params[0];
		php::array& update = params[1];
		if(!filter.is_array() || !filter.is_a_map() || !update.is_array()
			|| !update.is_a_map()) {
			throw php::exception("illegal filter / update, associate array is required");
		}

		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, this, filter, update, MONGOC_UPDATE_MULTI_UPDATE
		};
		ctx->req.data = ctx;
		if(params.length() > 2 && params[2].is_true()) {
			ctx->flags |= MONGOC_UPDATE_UPSERT;
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::update_wk, default_cb);
		return flame::async();
	}
	php::value collection::find_one(php::parameters& params) {
		php::array& filter = params[0], option(0);
		if(!filter.is_array() || !filter.is_a_map()) {
			throw php::exception("illegal selector, associate array is required");
		}
		if(params.length() > 1 && params[1].is_array()) {
			option.at("sort",4) = params[1];
		}
		option.at("limit", 5) = 1;
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, this, filter, option
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, collection_implement::find_one_wk, default_cb);
		return flame::async();
	}
	php::value collection::find_many(php::parameters& params) {
		php::array& filter = params[0], option(0);
		if(params.length() < 1 && !params[0].is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}
		if(params.length() > 1 && params[1].is_array()) {
			option.at("sort",4) = params[1];
		}
		if(params.length() > 2) {
			option.at("skip",4) = params[2].to_long();
		}
		if(params.length() > 3) {
			option.at("limit",5) = params[3].to_long();
		}
		if(params.length() > 4 && params[4].is_array()) {
			option.at("projection", 10) = params[4];
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, this, filter, option
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, collection_implement::find_many_wk, default_cb);
		return flame::async();
	}
}
}
}
