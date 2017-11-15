#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "client.h"
#include "collection.h"
#include "mongodb.h"
#include "bulk_result.h"
#include "cursor.h"


namespace flame {
namespace db {
namespace mongodb {
	void collection::init(client* cli,
		mongoc_client_t* client, mongoc_collection_t* collection) {
		this->worker_ = &cli->worker_;
		this->client_object = cli;
		this->client_       = client;
		this->collection_   = collection;
	}
	// TODO 拆分和精简下面 context 类型
	typedef struct collection_request_t {
		collection* self;
		php::value  rv; // 异步启动时作为引用，返回时作为结果
		coroutine*  co;
		bson_t*     doc1;
		bson_t*     doc2;
		mongoc_bulk_operation_t* bulk;
		mongoc_read_prefs_t*     pref;
		int         flags;
		uv_work_t   req;
	} collection_request_t;

	collection::collection()
	:collection_(nullptr) {

	}
	collection::~collection() {
		if(collection_) mongoc_collection_destroy(collection_);
	}
	php::value collection::__debugInfo(php::parameters& params) {
		php::array name(1);
		name["$name"] = php::string(mongoc_collection_get_name(collection_));
		return std::move(name);
	}
	void collection::default_cb(uv_work_t* w, int status) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		ctx->co->next(ctx->rv);
		delete ctx;
	}
	void collection::count_wk(uv_work_t* w) {
		bson_error_t error;
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		int64_t c = mongoc_collection_count(ctx->self->collection_,
			MONGOC_QUERY_SLAVE_OK, ctx->doc1, 0, 0, nullptr, &error);
		bson_destroy(ctx->doc1);
		if(c == -1) {
			ctx->rv = php::make_exception(error.message, error.code);
		}else{
			ctx->rv = c;
		}
	}
	php::value collection::count(php::parameters& params) {
		collection_request_t* ctx = new collection_request_t {
			this, this, coroutine::current, bson_new()
		};
		if(params.length() > 0) {
			fill_with(ctx->doc1, params[0]);
		}
		ctx->req.data = ctx;
		worker_->queue_work(&ctx->req, count_wk, default_cb);
		return flame::async();
	}
	void collection::insert_one_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		bson_error_t error;
		bool r = mongoc_collection_insert(
			ctx->self->collection_,
			MONGOC_INSERT_NONE,
			ctx->doc1,
			mongoc_client_get_write_concern(ctx->self->client_),
			&error);
		bson_destroy(ctx->doc1);
		if(r) {
			ctx->rv = bool(true);
		}else{
			ctx->rv = php::make_exception(error.message, error.code);
		}
	}
	php::value collection::insert_one(php::parameters& params) {
		if(params.length() < 1 || !params[0].is_array()) {
			throw php::exception("illegal document, associate array is required");
		}
		php::array& doc = params[0];
		if(!doc.is_a_map()) {
			throw php::exception("illegal document, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t
			{this, this, coroutine::current, bson_new(), nullptr};
		ctx->req.data = ctx;
		fill_with(ctx->doc1, doc);
		worker_->queue_work(&ctx->req, insert_one_wk, default_cb);
		return flame::async();
	}
	void collection::insert_many_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		bson_error_t error;
		bson_t       reply;
		uint32_t r = mongoc_bulk_operation_execute(ctx->bulk, &reply, &error);
		if(r == 0) {
			php::warn("insert_many failed: (%d) %s", error.code, error.message);
		}
		ctx->rv = bulk_result::create_from(r != 0, &reply);
		bson_destroy(&reply);
		mongoc_bulk_operation_destroy(ctx->bulk);
	}
	php::value collection::insert_many(php::parameters& params) {
		if(params.length() < 1 || !params[0].is_array()) {
			throw php::exception("illegal document, document array is required");
		}
		php::array& arr = params[0];
		if(!arr.is_a_list()) {
			throw php::exception("illegal document, array of documents is required");
		}
		bool ordered = false;
		if(params.length() >= 2) {
			ordered = params[2].is_true();
		}
		collection_request_t* ctx = new collection_request_t {
			this, this,
			coroutine::current,
			nullptr, nullptr,
			mongoc_collection_create_bulk_operation(
				collection_,
				ordered,
				mongoc_client_get_write_concern(ctx->self->client_)),
		};
		ctx->req.data = ctx;
		for(auto i=arr.begin(); i!= arr.end(); ++i) {
			if(!i->second.is_array()) {
				delete ctx;
				throw php::exception("illegal document, array of documents is required");
			}
			php::array& doc = i->second;
			if(!doc.is_a_map()) {
				delete ctx;
				throw php::exception("illegal document, array of documents is required");
			}
			bson_t item;
			bson_init(&item);
			fill_with(&item, i->second);
			mongoc_bulk_operation_insert(ctx->bulk, &item);
			bson_destroy(&item);
		}
		worker_->queue_work(&ctx->req, insert_many_wk, default_cb);
		return flame::async();
	}
	void collection::remove_one_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		bson_error_t error;
		bool r = mongoc_collection_remove(
			ctx->self->collection_,
			MONGOC_REMOVE_SINGLE_REMOVE,
			ctx->doc1,
			mongoc_client_get_write_concern(ctx->self->client_),
			&error);
		bson_destroy(ctx->doc1);
		if(r) {
			ctx->rv = true;
		}else{
			ctx->rv = php::make_exception(error.message, error.code);
		}
	}
	php::value collection::remove_one(php::parameters& params) {
		if(params.length() < 1 || !params[0].is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			this, this,
			coroutine::current,
			bson_new(),
		};
		ctx->req.data = ctx;
		fill_with(ctx->doc1, params[0]);
		worker_->queue_work(&ctx->req, remove_one_wk, default_cb);
		return flame::async();
	}

	void collection::remove_many_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		bson_error_t error;
		bool r = mongoc_collection_remove(
			ctx->self->collection_,
			MONGOC_REMOVE_NONE,
			ctx->doc1,
			mongoc_client_get_write_concern(ctx->self->client_),
			&error);
		bson_destroy(ctx->doc1);
		if(r) {
			ctx->rv = true;
		}else{
			ctx->rv = php::make_exception(error.message, error.code);
		}
	}
	php::value collection::remove_many(php::parameters& params) {
		if(params.length() < 1 || !params[0].is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t
			{ this, this, coroutine::current, bson_new() };
		ctx->req.data = ctx;
		fill_with(ctx->doc1, params[0]);
		worker_->queue_work(&ctx->req, remove_many_wk, default_cb);
		return flame::async();
	}
	void collection::update_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		bson_error_t error;
		bool r = mongoc_collection_update(ctx->self->collection_,
			(mongoc_update_flags_t)ctx->flags,
			ctx->doc1, // filter
			ctx->doc2, // update
			mongoc_client_get_write_concern(ctx->self->client_),
			&error);
		bson_destroy(ctx->doc1);
		bson_destroy(ctx->doc2);
		if(r) {
			ctx->rv = true;
		}else{
			ctx->rv = php::make_exception(error.message, error.code);
		}
	}
	php::value collection::update_one(php::parameters& params) {
		if(params.length() < 1 || !params[0].is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}else if(params.length() < 2 || !params[1].is_array()) {
			throw php::exception("illegal updates, associate array is required");
		}
		php::array& filter = params[0];
		php::array& update = params[1];
		if(!filter.is_a_map() || !filter.is_a_map()) {
			throw php::exception("illegal selector/update, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t
			{ this, this, coroutine::current, bson_new(), bson_new(), nullptr, nullptr, MONGOC_UPDATE_NONE };
		ctx->req.data = ctx;
		fill_with(ctx->doc1, filter);
		fill_with(ctx->doc2, update);
		if(params.length() >= 3 && params[2].is_true()) {
			ctx->flags |= MONGOC_UPDATE_UPSERT;
		}
		worker_->queue_work(&ctx->req, update_wk, default_cb);
		return flame::async();
	}
	php::value collection::update_many(php::parameters& params) {
		if(params.length() < 1 || !params[0].is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}else if(params.length() < 2 || !params[1].is_array()) {
			throw php::exception("illegal updates, associate array is required");
		}
		php::array& filter = params[0];
		php::array& update = params[1];
		if(!filter.is_a_map() || !filter.is_a_map()) {
			throw php::exception("illegal selector/update, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t{
			this, this,
			coroutine::current,
			bson_new(),
			bson_new(),
			nullptr,
			nullptr,
			MONGOC_UPDATE_MULTI_UPDATE,
		};
		ctx->req.data = ctx;
		fill_with(ctx->doc1, filter);
		fill_with(ctx->doc2, update);
		if(params.length() >= 3 && params[2].is_true()) {
			ctx->flags |= MONGOC_UPDATE_UPSERT;
		}
		worker_->queue_work(&ctx->req, update_wk, default_cb);
		return flame::async();
	}
	void collection::find_one_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		mongoc_cursor_t* cs = mongoc_collection_find_with_opts(ctx->self->collection_,
			ctx->doc1, // query
			ctx->doc2, // opts
			ctx->pref
		);
		bson_destroy(ctx->doc1);
		bson_destroy(ctx->doc2);
		const bson_t* doc;
		ctx->rv = php::array(0);
		while (mongoc_cursor_next(cs, &doc)) {
			ctx->rv = php::array(0);
			fill_with(ctx->rv, doc);
			// break;
		}
		mongoc_cursor_destroy(cs);
	}
	php::value collection::find_one(php::parameters& params) {
		if(params.length() < 1 && !params[0].is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}
		php::array opts;
		opts["limit"] = 1;
		if(params.length() > 1 && params[1].is_array()) {
			opts["sort"] = params[1];
		}
		collection_request_t* ctx = new collection_request_t
			{ this, this, coroutine::current, bson_new(), bson_new(), nullptr };
		ctx->req.data = ctx;
		fill_with(ctx->doc1, params[0]);
		fill_with(ctx->doc2, opts);
		worker_->queue_work(&ctx->req, find_one_wk, default_cb);
		return flame::async();
	}
	void collection::find_many_wk(uv_work_t* w) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		mongoc_cursor_t* cs = mongoc_collection_find_with_opts(ctx->self->collection_,
			ctx->doc1, // query
			ctx->doc2, // fields
			ctx->pref
		);
		bson_destroy(ctx->doc1);
		bson_destroy(ctx->doc2);
		php::object obj = php::object::create<cursor>();
		cursor*     cpp = obj.native<cursor>();
		cpp->init(ctx->self, cs);

		ctx->rv = std::move(obj);
	}
	php::value collection::find_many(php::parameters& params) {
		if(params.length() < 1 && !params[0].is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}
		php::array opts;
		if(params.length() > 1 && params[1].is_array()) {
			opts["sort"] = params[1];
		}
		if(params.length() > 2) {
			opts["skip"] = params[2].to_long();
		}
		if(params.length() > 3) {
			opts["limit"]  = params[3].to_long();
		}
		if(params.length() > 4 && params[4].is_array()) {
			opts["projection"] = params[4];
		}
		collection_request_t* ctx = new collection_request_t
			{ this, this, coroutine::current, bson_new(), bson_new(), nullptr };
		ctx->req.data = ctx;
		fill_with(ctx->doc1, params[0]);
		fill_with(ctx->doc2, opts);
		worker_->queue_work(&ctx->req, find_many_wk, default_cb);
		return flame::async();
	}
}
}
}
