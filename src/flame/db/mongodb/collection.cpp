#include "../../fiber.h"
#include "collection.h"
#include "mongodb.h"
#include "bulk_result.h"

namespace flame {
namespace db {
namespace mongodb {
	//
	typedef struct {
		collection* self;
		fiber*      fib;
		bson_t*     doc1;
		bson_t*     doc2;
		mongoc_bulk_operation_t* bulk;
		mongoc_read_prefs_t*     pref;
		int         flags;
		uv_work_t   req;
		php::value  rv;
	} collection_request;
	collection::~collection() {
		if(collection_) mongoc_collection_destroy(collection_);
	}
	php::value collection::__debugInfo(php::parameters& params) {
		php::array name(1);
		name["$name"] = php::string(mongoc_collection_get_name(collection_));
		return std::move(name);
	}
	void collection::default_cb(uv_work_t* w, int status) {
		collection_request* cq = reinterpret_cast<collection_request*>(w->data);
		cq->fib->next(cq->rv);
		delete cq;
	}

	void collection::count_wk(uv_work_t* w) {
		bson_error_t error;
		collection_request* cq = reinterpret_cast<collection_request*>(w->data);
		int64_t c = mongoc_collection_count(cq->self->collection_,
			MONGOC_QUERY_SLAVE_OK, cq->doc1, 0, 0, nullptr, &error);
		bson_destroy(cq->doc1);
		if(c == -1) {
			cq->rv = php::make_exception(error.message, error.code);
		}else{
			cq->rv = c;
		}
	}
	php::value collection::count(php::parameters& params) {
		collection_request* cq = new collection_request {this, flame::this_fiber(), bson_new()};
		cq->req.data = cq;
		flame::this_fiber()->push();
		uv_queue_work(flame::loop, &cq->req, count_wk, default_cb);
		return flame::async();
	}

	void collection::insert_one_wk(uv_work_t* w) {
		collection_request* cq = reinterpret_cast<collection_request*>(w->data);
		bson_error_t error;
		bool r = mongoc_collection_insert(
			cq->self->collection_,
			MONGOC_INSERT_NONE,
			cq->doc1,
			mongoc_client_get_write_concern(cq->self->client_),
			&error);
		bson_destroy(cq->doc1);
		if(r) {
			cq->rv = bool(true);
		}else{
			cq->rv = php::make_exception(error.message, error.code);
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
		collection_request* cq = new collection_request
			{this, flame::this_fiber(), bson_new(), nullptr};
		cq->req.data = cq;
		fill_bson_with(cq->doc1, doc);
		flame::this_fiber()->push();
		uv_queue_work(flame::loop, &cq->req, insert_one_wk, default_cb);
		return flame::async();
	}

	void collection::insert_many_wk(uv_work_t* w) {
		collection_request* cq = reinterpret_cast<collection_request*>(w->data);
		bson_error_t error;
		bson_t       reply;
		uint32_t r = mongoc_bulk_operation_execute(cq->bulk, &reply, &error);
		if(r == 0) {
			php::warn("insert_many failed: (%d) %s", error.code, error.message);
		}
		cq->rv = bulk_result::create_from(r != 0, &reply);
		bson_destroy(&reply);
		mongoc_bulk_operation_destroy(cq->bulk);
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
		collection_request* cq = new collection_request {
			this,
			flame::this_fiber(),
			nullptr, nullptr,
			mongoc_collection_create_bulk_operation(
				collection_,
				ordered,
				mongoc_client_get_write_concern(cq->self->client_)),
		};
		cq->req.data = cq;
		for(auto i=arr.begin(); i!= arr.end(); ++i) {
			if(!i->second.is_array()) {
				delete cq;
				throw php::exception("illegal document, array of documents is required");
			}
			php::array& doc = i->second;
			if(!doc.is_a_map()) {
				delete cq;
				throw php::exception("illegal document, array of documents is required");
			}
			bson_t item;
			bson_init(&item);
			fill_bson_with(&item, i->second);
			mongoc_bulk_operation_insert(cq->bulk, &item);
			bson_destroy(&item);
		}
		flame::this_fiber()->push();
		uv_queue_work(flame::loop, &cq->req, insert_many_wk, default_cb);
		return flame::async();
	}
	void collection::remove_one_wk(uv_work_t* w) {
		collection_request* cq = reinterpret_cast<collection_request*>(w->data);
		bson_error_t error;
		bool r = mongoc_collection_remove(
			cq->self->collection_,
			MONGOC_REMOVE_SINGLE_REMOVE,
			cq->doc1,
			mongoc_client_get_write_concern(cq->self->client_),
			&error);
		bson_destroy(cq->doc1);
		if(r) {
			cq->rv = true;
		}else{
			cq->rv = php::make_exception(error.message, error.code);
		}
	}
	php::value collection::remove_one(php::parameters& params) {
		if(params.length() < 1 || !params[0].is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}
		collection_request* cq = new collection_request {
			this,
			flame::this_fiber(),
			bson_new(),
		};
		cq->req.data = cq;
		fill_bson_with(cq->doc1, params[0]);
		flame::this_fiber()->push();
		uv_queue_work(flame::loop, &cq->req, remove_one_wk, default_cb);
		return flame::async();
	}

	void collection::remove_many_wk(uv_work_t* w) {
		collection_request* cq = reinterpret_cast<collection_request*>(w->data);
		bson_error_t error;
		bool r = mongoc_collection_remove(
			cq->self->collection_,
			MONGOC_REMOVE_NONE,
			cq->doc1,
			mongoc_client_get_write_concern(cq->self->client_),
			&error);
		bson_destroy(cq->doc1);
		if(r) {
			cq->rv = true;
		}else{
			cq->rv = php::make_exception(error.message, error.code);
		}
	}
	php::value collection::remove_many(php::parameters& params) {
		if(params.length() < 1 || !params[0].is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}
		collection_request* cq = new collection_request
			{ this, flame::this_fiber(), bson_new() };
		cq->req.data = cq;
		fill_bson_with(cq->doc1, params[0]);
		flame::this_fiber()->push();
		uv_queue_work(flame::loop, &cq->req, remove_many_wk, default_cb);
		return flame::async();
	}

	void collection::update_wk(uv_work_t* w) {
		collection_request* cq = reinterpret_cast<collection_request*>(w->data);
		bson_error_t error;
		bool r = mongoc_collection_update(cq->self->collection_,
			(mongoc_update_flags_t)cq->flags,
			cq->doc1, // filter
			cq->doc2, // update
			mongoc_client_get_write_concern(cq->self->client_),
			&error);
		bson_destroy(cq->doc1);
		bson_destroy(cq->doc2);
		if(r) {
			cq->rv = true;
		}else{
			cq->rv = php::make_exception(error.message, error.code);
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
		collection_request* cq = new collection_request
			{ this, flame::this_fiber(), bson_new(), bson_new(), nullptr, nullptr, MONGOC_UPDATE_NONE };
		cq->req.data = cq;
		fill_bson_with(cq->doc1, filter);
		fill_bson_with(cq->doc2, update);
		if(params.length() >= 3 && params[2].is_true()) {
			cq->flags |= MONGOC_UPDATE_UPSERT;
		}
		flame::this_fiber()->push();
		uv_queue_work(flame::loop, &cq->req, update_wk, default_cb);
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
		collection_request* cq = new collection_request{
			this,
			flame::this_fiber(),
			bson_new(),
			bson_new(),
			nullptr,
			nullptr,
			MONGOC_UPDATE_MULTI_UPDATE,
		};
		cq->req.data = cq;
		fill_bson_with(cq->doc1, filter);
		fill_bson_with(cq->doc2, update);
		if(params.length() >= 3 && params[2].is_true()) {
			cq->flags |= MONGOC_UPDATE_UPSERT;
		}
		flame::this_fiber()->push();
		uv_queue_work(flame::loop, &cq->req, update_wk, default_cb);
		return flame::async();
	}

	void collection::find_one_wk(uv_work_t* w) {
		collection_request* cq = reinterpret_cast<collection_request*>(w->data);
		mongoc_cursor_t* cs = mongoc_collection_find_with_opts(cq->self->collection_,
			cq->doc1, // query
			cq->doc2, // opts
			cq->pref
		);
		bson_destroy(cq->doc1);
		bson_destroy(cq->doc2);
		const bson_t* doc;
		while (mongoc_cursor_next(cs, &doc)) {
			char* str = bson_as_canonical_extended_json (doc, NULL);
			printf ("--> %s\n", str);
			bson_free (str);
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
		collection_request* cq = new collection_request
			{ this, flame::this_fiber(), bson_new(), bson_new(), nullptr };
		cq->req.data = cq;
		fill_bson_with(cq->doc1, params[0]);
		fill_bson_with(cq->doc2, opts);
		flame::this_fiber()->push();
		uv_queue_work(flame::loop, &cq->req, find_one_wk, default_cb);
		return flame::async();
	}

	void collection::find_many_wk(uv_work_t* w) {
		collection_request* cq = reinterpret_cast<collection_request*>(w->data);
		mongoc_cursor_t* cs = mongoc_collection_find_with_opts(cq->self->collection_,
			cq->doc1, // query
			cq->doc2, // fields
			cq->pref
		);
		bson_destroy(cq->doc1);
		bson_destroy(cq->doc2);
		const bson_t* doc;
		while (mongoc_cursor_next(cs, &doc)) {
			char* str = bson_as_canonical_extended_json (doc, NULL);
			printf ("--> %s\n", str);
			bson_free (str);
		}
		mongoc_cursor_destroy(cs);
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
		collection_request* cq = new collection_request
			{ this, flame::this_fiber(), bson_new(), bson_new(), nullptr };
		cq->req.data = cq;
		fill_bson_with(cq->doc1, params[0]);
		fill_bson_with(cq->doc2, opts);
		flame::this_fiber()->push();
		uv_queue_work(flame::loop, &cq->req, find_many_wk, default_cb);
		return flame::async();
	}
}
}
}
