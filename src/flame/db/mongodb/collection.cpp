#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "client.h"
#include "collection_implement.h"
#include "collection.h"
#include "mongodb.h"
#include "cursor_implement.h"
#include "cursor.h"
#include "bulk_result.h"


namespace flame {
namespace db {
namespace mongodb {
	void collection::init(thread_worker* worker, client* cli, mongoc_collection_t* col) {
		impl = new collection_implement(worker, this, col);
		ref_ = cli;
	}
	php::value collection::__destruct(php::parameters& params) {
		collection_request_t* ctx = new collection_request_t {
			nullptr, impl, nullptr
		};
		ctx->req.data = ctx;
		// 这里不能使用 close_work ，实际关闭在 client 中
		impl->worker_->queue_work(&ctx->req, collection_implement::close_wk, default_cb);
		return nullptr;
	}
	void collection::default_cb(uv_work_t* w, int status) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		if(ctx->co) ctx->co->next(ctx->rv);
		delete ctx;
	}
	void collection::boolean_cb(uv_work_t* w, int status) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		if(ctx->rv.is_pointer()) {
			bson_error_t* error = ctx->rv.ptr<bson_error_t>();
			ctx->co->fail(error->message, error->code);
			delete error;
		}else{
			ctx->co->next(ctx->rv);
		}
		delete ctx;
	}
	php::value collection::count(php::parameters& params) {
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr
		};
		ctx->req.data = ctx;
		if(params.length() > 0 && params[0].is_array()) {
			ctx->doc1 = params[0];
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::count_wk, boolean_cb);
		return flame::async(this);
	}
	php::value collection::insert_one(php::parameters& params) {
		php::array& doc = params[0];
		if(!doc.is_array() || !doc.is_a_map()) {
			throw php::exception("illegal document, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, doc};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, collection_implement::insert_one_wk, boolean_cb);
		return flame::async(this);
	}
	php::value collection::insert_many(php::parameters& params) {
		php::array& docs = params[0];
		if(!docs.is_array() || !docs.is_a_list()) {
			throw php::exception("illegal document, document array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, docs, php::array(nullptr)
		};
		ctx->req.data = ctx;
		if(params.length() > 1 && params[1].is_array()) {
			ctx->doc2 = params[1];
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::insert_many_wk, insert_many_cb);
		return flame::async(this);
	}
	void collection::insert_many_cb(uv_work_t* w, int status) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		if(ctx->rv.is_pointer()) {
			bson_t* reply = ctx->rv.ptr<bson_t>();
			ctx->rv = bulk_result::create_from(ctx->flags == 0, reply);
			bson_destroy(reply);
		}
		ctx->co->next(ctx->rv);
		delete ctx;
	}
	php::value collection::remove_one(php::parameters& params) {
		php::array& filter = params[0];
		if(!filter.is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, filter
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, collection_implement::remove_one_wk, boolean_cb);
		return flame::async(this);
	}
	php::value collection::remove_many(php::parameters& params) {
		php::array& filter = params[0];
		if(!filter.is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, filter
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, collection_implement::remove_many_wk, boolean_cb);
		return flame::async(this);
	}
	php::value collection::update_one(php::parameters& params) {
		php::array& filter = params[0];
		php::array& update = params[1];
		if(!filter.is_array() || !filter.is_a_map() || !update.is_array()
			|| !update.is_a_map()) {
			throw php::exception("illegal filter / update, associate array is required");
		}

		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, filter, update, MONGOC_UPDATE_NONE
		};
		ctx->req.data = ctx;
		if(params.length() > 2 && params[2].is_true()) {
			ctx->flags |= MONGOC_UPDATE_UPSERT;
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::update_wk, boolean_cb);
		return flame::async(this);
	}
	php::value collection::update_many(php::parameters& params) {
		php::array& filter = params[0];
		php::array& update = params[1];
		if(!filter.is_array() || !filter.is_a_map() || !update.is_array()
			|| !update.is_a_map()) {
			throw php::exception("illegal filter / update, associate array is required");
		}

		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, filter, update, MONGOC_UPDATE_MULTI_UPDATE
		};
		ctx->req.data = ctx;
		if(params.length() > 2 && params[2].is_true()) {
			ctx->flags |= MONGOC_UPDATE_UPSERT;
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::update_wk, boolean_cb);
		return flame::async(this);
	}
	php::value collection::find_one(php::parameters& params) {
		php::array& filter = params[0], option;
		if(!filter.is_array() || !filter.is_a_map()) {
			throw php::exception("illegal selector, associate array is required");
		}
		if(params.length() > 1 && params[1].is_array()) {
			option = params[1];
		}else{
			option = php::array(0);
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, filter, option
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, collection_implement::find_one_wk, find_one_cb);
		return flame::async(this);
	}
	void collection::find_one_cb(uv_work_t* w, int status) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		if(ctx->doc1.is_pointer()) {
			const bson_t* doc = ctx->doc1.ptr<const bson_t>();
			php::array rv(0);
			fill_with(rv, doc);
			ctx->rv = std::move(rv);
			// 销毁 doc2 中保存的 cursor
			ctx->self->worker_->queue_work(&ctx->req, collection_implement::find_one_af, default_cb);
		}else{
			ctx->co->next();
			delete ctx;
		}
	}
	php::value collection::find_many(php::parameters& params) {
		if(params.length() < 1 && !params[0].is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}
		php::array& filter = params[0], option;
		if(params.length() > 1 && params[1].is_array()) {
			option = params[1];
		}else{
			option = php::array(0);
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, filter, option
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, collection_implement::find_many_wk, find_many_cb);
		return flame::async(this);
	}
	void collection::find_many_cb(uv_work_t* w, int status) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		if(ctx->doc1.is_pointer()) {
			mongoc_cursor_t* cs = ctx->doc1.ptr<mongoc_cursor_t>();
			php::object obj = php::object::create<cursor>();
			cursor*     cpp = obj.native<cursor>();
			cpp->init(ctx->self->worker_, ctx->self->cpp_, cs);
			ctx->rv = std::move(obj);
		}
		ctx->co->next(ctx->rv);
		delete ctx;
	}
}
}
}
