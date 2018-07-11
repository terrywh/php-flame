#include "../coroutine.h"
#include "collection.h"
#include "_connection_base.h"
#include "_connection_pool.h"
#include "_connection_lock.h"
#include "mongodb.h"
#include "cursor.h"
#include "collection.h"

namespace flame {
namespace mongodb {
	void collection::declare(php::extension_entry& ext) {
		php::class_entry<collection> class_collection("flame\\mongodb\\collection");
		class_collection
			.property({"name", ""})
			.method<&collection::__construct>("__construct", {}, php::PRIVATE)
			.method<&collection::insert>("insert", {
				{"data", php::TYPE::ARRAY},
				{"ordered", php::TYPE::BOOLEAN, false, true} // true
			})
			.method<&collection::delete_>("delete", {
				{"query", php::TYPE::ARRAY},
				{"limit", php::TYPE::INTEGER, false, true}, // 0
			})
			.method<&collection::update>("update", {
				{"query", php::TYPE::ARRAY},
				{"update", php::TYPE::ARRAY},
				{"upsert", php::TYPE::BOOLEAN, false, true}, // false
			})
			.method<&collection::find>("find", {
				{"filter", php::TYPE::ARRAY},
				{"projection", php::TYPE::ARRAY, false, true},
				{"sort", php::TYPE::ARRAY, false, true},
				{"limit", php::TYPE::ARRAY, false, true},
			})
			.method<&collection::one>("one", {
				{"filter", php::TYPE::ARRAY},
				{"sort", php::TYPE::ARRAY, false, true},
			})
			.method<&collection::get>("get", {
				{"field", php::TYPE::STRING},
				{"filter", php::TYPE::ARRAY},
				{"sort", php::TYPE::ARRAY, false, true},
			})
			.method<&collection::count>("count", {
				{"query", php::TYPE::ARRAY},
			})
			.method<&collection::aggregate>("aggregate", {
				{"pipeline", php::TYPE::ARRAY},
			});
		ext.add(std::move(class_collection));
	}
	php::value collection::insert(php::parameters& params) {
		php::array cmd(8);
		cmd.set("insert", c_);
		php::array docs = params[0];
		if(!docs.exists(0)) { // 单项插入的情况
			docs = php::array(2);
			docs.set(0, params[0]);
		}
		cmd.set("documents", docs);
		if(params.size() > 1) {
			cmd.set("ordered", params[1].to_boolean());
		}
		p_->execute(coroutine::current, php::object(this), convert(cmd), true);
		return coroutine::async();
	}
	php::value collection::delete_(php::parameters& params) {
		php::array cmd(8);
		cmd.set("delete", c_);
		php::array deletes(2), deleteo(2);
		deleteo.set("q", params[0]);
		if(params.size() > 1 && params[1].typeof(php::TYPE::INTEGER)) {
			deleteo.set("limit", params[1].to_integer());
		}else{
			deleteo.set("limit", 0);
		}
		deletes.set(0, deleteo);
		cmd.set("deletes", deletes);
		p_->execute(coroutine::current, php::object(this), convert(cmd), true);
		return coroutine::async();
	}
	php::value collection::update(php::parameters& params) {
		php::array cmd(8);
		cmd.set("update", c_);
		php::array updates(2), updateo(4);
		updateo.set("q", params[0]);
		updateo.set("u", params[1]);
		if(params.size() > 2) {
			updateo.set("upsert", params[2].to_boolean());
		}
		updateo.set("multi", true);
		updates.set(0, updateo);
		cmd.set("updates", updates);
		p_->execute(coroutine::current, php::object(this), convert(cmd), true);
		return coroutine::async();
	}
	php::value collection::find(php::parameters& params) {
		php::array cmd(8);
		cmd.set("find", c_);
		if(params.size() > 0) {
			cmd.set("filter", params[0]);
		}
		if(params.size() > 1) {
			php::array project;
			if(params[1].typeof(php::TYPE::ARRAY)) {
				php::array fields = params[1];
				if(fields.exists(0)) { // 将纯字段列表形式转换为K/V形式
					project = php::array(8);
					for(auto i=fields.begin(); i!=fields.end(); ++i) {
						project.set(php::string(i->second), 1);
					}
				}else{
					project = fields;
				}
			}else if(params[1].typeof(php::TYPE::STRING)) { // 单个字段
				php::array project(2);
				project.set(php::string(params[1]), 1);
			}else{
				goto PROJECTION_ALL;
			}
			if(!project.exists("_id")) {
				project.set("_id", 0);
			}
			cmd.set("projection", project);
		}
PROJECTION_ALL:
		if(params.size() > 2 && params[2].typeof(php::TYPE::ARRAY)) {
			cmd.set("sort", params[2]);
		}
		if(params.size() > 3) {
			if(params[3].typeof(php::TYPE::INTEGER)) {
				cmd.set("limit", params[3]);
			}else if(params[3].typeof(php::TYPE::ARRAY)) {
				php::array limits = params[3];
				if(limits.size() > 0) cmd.set("skip",  limits[0].to_integer());
				if(limits.size() > 1) cmd.set("limit", limits[1].to_integer());
			}
		}
		p_->execute(coroutine::current, php::object(this), convert(cmd));
		return coroutine::async();
	}
	php::value collection::one(php::parameters& params) {
		php::array cmd(8);
		cmd.set("find", c_);
		cmd.set("filter", params[0]);
		if(params.size() > 1 && params[1].typeof(php::TYPE::ARRAY)) {
			cmd.set("sort", params[1]);
		}
		cmd.set("limit", 1);
		php::object ref(this);
		coroutine::current->stack(php::callable([] (php::parameters& params) -> php::value {
			php::object cs = params[0];
			if(cs.instanceof(php::class_entry<cursor>::entry())) return cs.call("next");
			return nullptr;
		}), ref);
		p_->execute(coroutine::current, ref, convert(cmd));
		return coroutine::async();
	}
	php::value collection::get(php::parameters& params) {
		php::array cmd(8);
		cmd.set("find", c_);
		php::array project(2);
		php::string field = params[0];
		project.set(field, 1);
		cmd.set("projection", project);
		cmd.set("filter", params[1]);
		if(params.size() > 2 && params[2].typeof(php::TYPE::ARRAY)) {
			cmd.set("sort", params[2]);
		}
		cmd.set("limit", 1);
		php::object ref(this);
		coroutine::current
			->stack(php::callable([field] (php::parameters& params) -> php::value {
				php::array doc = params[0];
				if(doc.typeof(php::TYPE::ARRAY)) return doc.get(field);
				else return nullptr;
			}))
			->stack(php::callable([] (php::parameters& params) -> php::value {
				php::object cs = params[0];
				if(cs.instanceof(php::class_entry<cursor>::entry())) return cs.call("next");
				return nullptr;
			}), ref);
		p_->execute(coroutine::current, ref, convert(cmd));
		return coroutine::async();
	}
	php::value collection::count(php::parameters& params) {
		php::array cmd(8);
		cmd.set("count", c_);
		cmd.set("query", params[0]);
		php::object ref(this);
		coroutine::current->stack(php::callable([] (php::parameters& params) -> php::value {
			php::array doc = params[0];
			if(doc.typeof(php::TYPE::ARRAY)) return doc.get("n");
			else return nullptr;
		}), ref);
		p_->execute(coroutine::current, ref, convert(cmd));
		return coroutine::async();
	}
	php::value collection::aggregate(php::parameters& params) {
		php::array cmd(8);
		cmd.set("aggregate", c_);
		php::array pipeline = params[0];
		if(!pipeline.exists(0)) throw php::exception(zend_ce_type_error, "aggregate pipeline must be a array of stages (array)");
		cmd.set("pipeline", params[0]);
		cmd.set("cursor", php::array(0));
		p_->execute(coroutine::current, php::object(this), convert(cmd));
		return coroutine::async();
	}
}
}