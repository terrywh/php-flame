#include "../../vendor.h"
#include "header.h"

namespace net { namespace http {
	void header::init(php::extension_entry& extension) {
		php::class_entry<header> class_header("flame\\net\\http\\header");
		class_header.implements("Iterator");
		class_header.add<&header::current>("current");
		class_header.add<&header::key>("key");
		class_header.add<&header::next>("next");
		class_header.add<&header::rewind>("rewind");
		class_header.add<&header::valid>("valid");
		class_header.implements("ArrayAccess");
		class_header.add<&header::offsetExists>("offsetExists", {
			php::of_mixed("offset"),
		});
		class_header.add<&header::offsetGet>("offsetGet", {
			php::of_mixed("offset"),
		});
		class_header.add<&header::offsetSet>("offsetSet", {
			php::of_mixed("offset"),
			php::of_mixed("value"),
		});
		class_header.add<&header::offsetUnset>("offsetUnset", {
			php::of_mixed("offset"),
		});
		class_header.add<&header::append>("append");
		class_header.add<&header::remove>("remove");
		extension.add(std::move(class_header));
	}
	void header::init(evkeyvalq* headers) {
		queue_ = headers;
		item_  = TAILQ_FIRST(queue_);
	}
	// Iterator
	php::value header::current(php::parameters& params) {
		return php::value(item_->value, std::strlen(item_->value));
	}
	// Iterator
	php::value header::key(php::parameters& params) {
		return php::value(item_->key, std::strlen(item_->key));
	}
	// Iterator
	php::value header::next(php::parameters& params) {
		item_ = TAILQ_NEXT(item_, next);
		return nullptr;
	}
	// Iterator
	php::value header::rewind(php::parameters& params) {
		item_ = TAILQ_FIRST(queue_);
		return nullptr;
	}
	// Iterator
	php::value header::valid(php::parameters& params) {
		return item_ != nullptr;
	}
	// ArrayAccess
	php::value header::offsetExists(php::parameters& params) {
		zend_string* key = params[0];
		return evhttp_find_header(queue_, key->val) != nullptr;
	}
	// ArrayAccess
	php::value header::offsetGet(php::parameters& params) {
		zend_string* key = params[0];
		const char*  val = evhttp_find_header(queue_, key->val);
		return php::value(val, std::strlen(val));
	}
	// ArrayAccess
	php::value header::offsetSet(php::parameters& params) {
		zend_string* key = params[0];
		while(evhttp_remove_header(queue_, key->val) == 0) ;
		zend_string* val = params[1];
		if(evhttp_add_header(queue_, key->val, val->val) == -1) {
			throw php::exception("header set failed: illegal key/val");
			return false;
		}else{
			return true;
		}
	}
	// ArrayAccess
	php::value header::offsetUnset(php::parameters& params) {
		zend_string* key = params[0];
		while(evhttp_remove_header(queue_, key->val) == 0) ;
		return nullptr;
	}

	php::value header::append(php::parameters& params) {
		zend_string* key = params[0];
		zend_string* val = params[1];

		if(evhttp_add_header(queue_, key->val, val->val) == -1) {
			throw php::exception("header set failed: illegal key/val");
			return false;
		}else{
			return true;
		}
	}

	php::value header::remove(php::parameters& params) {
		return offsetUnset(params);
	}
}}
