#include "../coroutine.h"
#include "client.h"
#include "_connection_base.h"
#include "_connection_pool.h"
#include "mongodb.h"
#include "cursor.h"
#include "collection.h"

namespace flame {
namespace mongodb {
	void client::declare(php::extension_entry& ext) {
		php::class_entry<client> class_client("flame\\mongodb\\client");
		class_client
			.method<&client::__construct>("__construct", {}, php::PRIVATE)
			.method<&client::execute>("execute", {
				{"command", php::TYPE::ARRAY},
				{"write", php::TYPE::BOOLEAN, false, true},
			})
			.method<&client::__get>("collection", {
				{"name", php::TYPE::STRING}
			})
			.method<&client::__get>("__get", {
				{"name", php::TYPE::STRING}
			})
			.method<&client::__isset>("__isset", {
				{"name", php::TYPE::STRING}
			});
		ext.add(std::move(class_client));
	}
	php::value client::execute(php::parameters& params) {
		bool write = false;
		if(params.size() > 1) write = params[1].to_boolean();
		p_->execute(coroutine::current, php::object(this), convert(params[0]), write);
		return coroutine::async();
	}
	php::value client::__get(php::parameters& params) {
		php::object col(php::class_entry<collection>::entry());
		collection* col_ = static_cast<collection*>(php::native(col));
		col_->p_ = p_;
		col_->c_ = params[0];
		col.set("name", params[0]);
		return std::move(col);
	}
	php::value client::__isset(php::parameters& params) {
		return true;
	}
}
}