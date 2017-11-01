#include "../../coroutine.h"
#include "client.h"
#include "collection.h"

namespace flame {
namespace db {
namespace mongodb {
	php::value client::__construct(php::parameters& params) {
		if(params.length() >= 1 && params[0].is_string()) {
			php::string& uri = params[0];
			uri_ = mongoc_uri_new(uri.c_str());
		}
		if(!uri_) {
			throw php::exception("failed to construct mongodb client: failed to parse URI");
		}
		client_ = mongoc_client_new_from_uri(uri_);
		if(!client_) {
			throw php::exception("failed to construct mongodb client");
		}
		mongoc_client_set_error_api(client_, 2);
		return nullptr;
	}
	php::value client::__destruct(php::parameters& params) {
		if(client_) mongoc_client_destroy(client_);
		return nullptr;
	}
	php::value client::collection(php::parameters& params) {
		if(params.length() >= 1 && params[0].is_string()) {
			return collection_by_name(params[0]);
		}else{
			throw php::exception("collection name must be of type 'string'");
		}
	}
	php::value client::close(php::parameters& params) {
		mongoc_client_destroy(client_);
		client_ = nullptr;
		return nullptr;
	}
	php::object client::collection_by_name(const php::string& name) {
		mongoc_collection_t* col = mongoc_client_get_collection(client_, mongoc_uri_get_database(uri_), name.c_str());
		php::object          obj = php::object::create<mongodb::collection>();
		mongodb::collection* cpp = obj.native<mongodb::collection>();
		cpp->init(this, client_, col);
		return std::move(obj);
	}
}
}
}
