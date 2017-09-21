#include "../../fiber.h"
#include "collection.h"

namespace flame {
namespace db {
namespace mongodb {
	collection::~collection() {
		if(collection_) mongoc_collection_destroy(collection_);
	}
	void collection::count_work(flame::fiber* fiber, void* data) {
		bson_error_t error;
		collection* self = reinterpret_cast<collection*>(data);
		bson_t query;
		bson_init(&query);
		int64_t c = mongoc_collection_count(self->collection_, MONGOC_QUERY_NONE,
			&query, 0, 0, nullptr, &error);
		if(c == -1) {
			self->result_ = php::make_exception(error.message, error.code);
		}else{
			self->result_ = c;
		}
	}
	void collection::count_done(flame::fiber* fiber, void* data) {
		collection* self = reinterpret_cast<collection*>(data);
		fiber->next(self->result_);
	}
	php::value collection::count(php::parameters& params) {
		flame::queue(count_work, count_done, this);
		return flame::async();
	}
}
}
}
