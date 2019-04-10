#include "../coroutine.h"
#include "_connection_pool.h"
#include "collection.h"
#include "cursor.h"

namespace flame::mongodb {
    void collection::declare(php::extension_entry &ext) {
        php::class_entry<collection> class_collection("flame\\mongodb\\collection");
        class_collection
            .property({"name", ""})
            .method<&collection::__construct>("__construct", {}, php::PRIVATE)
            .method<&collection::insert>("insert",{
                {"data", php::TYPE::ARRAY},
                {"ordered", php::TYPE::BOOLEAN, false, true} // true
            })
            .method<&collection::insert>("insert_many", {
                {"data", php::TYPE::ARRAY},
                {"ordered", php::TYPE::BOOLEAN, false, true} // true
            })
            .method<&collection::insert>("insert_one", {
                {"data", php::TYPE::ARRAY},
            })
            .method<&collection::delete_>("delete", {
                {"query", php::TYPE::ARRAY},
                {"limit", php::TYPE::INTEGER, false, true}, // 0
            })
            .method<&collection::delete_>("delete_many", {
                {"query", php::TYPE::ARRAY},
                {"limit", php::TYPE::INTEGER, false, true}, // 0
            })
            .method<&collection::delete_>("delete_one", {
                {"query", php::TYPE::ARRAY},
            })
            .method<&collection::update>("update", {
                {"query", php::TYPE::ARRAY},
                {"update", php::TYPE::ARRAY},
                {"upsert", php::TYPE::BOOLEAN, false, true}, // false
            })
            .method<&collection::update>("update_many", {
                {"query", php::TYPE::ARRAY},
                {"update", php::TYPE::ARRAY},
                {"upsert", php::TYPE::BOOLEAN, false, true}, // false
            })
            .method<&collection::update_one>("update_one", {
                {"query", php::TYPE::ARRAY},
                {"update", php::TYPE::ARRAY},
                {"upsert", php::TYPE::BOOLEAN, false, true}, // false
            })
            .method<&collection::find>("find", {
                {"filter", php::TYPE::ARRAY},
                {"projection", php::TYPE::ARRAY, false, true},
                {"sort", php::TYPE::ARRAY, false, true},
                {"limit", php::TYPE::UNDEFINED, false, true},
            })
            .method<&collection::find_one>("one", { 
                {"filter", php::TYPE::ARRAY},
                {"sort", php::TYPE::ARRAY, false, true},
            })
            .method<&collection::find_one>("find_one", { // 兼容原 API 名称定义
                {"filter", php::TYPE::ARRAY},
                {"sort", php::TYPE::ARRAY, false, true},
            })
            .method<&collection::get>("get", {
                {"filter", php::TYPE::ARRAY},
                {"field", php::TYPE::STRING},
                {"sort", php::TYPE::ARRAY, false, true},
            })
            .method<&collection::count>("count", {
                {"query", php::TYPE::ARRAY},
            })
            .method<&collection::aggregate>("aggregate", {
                {"pipeline", php::TYPE::ARRAY},
            })
            .method<&collection::find_and_delete>("find_and_delete", {
                {"query", php::TYPE::ARRAY},
                {"sort", php::TYPE::ARRAY, false, true},
                {"upsert", php::TYPE::BOOLEAN, false, true},
                {"fields", php::TYPE::ARRAY, false, true},
            })
            .method<&collection::find_and_update>("find_and_update", {
                {"query", php::TYPE::ARRAY},
                {"update", php::TYPE::ARRAY},
                {"sort", php::TYPE::ARRAY, false, true},
                {"upsert", php::TYPE::BOOLEAN, false, true},
                {"fields", php::TYPE::ARRAY, false, true},
                {"new", php::TYPE::BOOLEAN, false, true},
            });
        ext.add(std::move(class_collection));
    }

    php::value collection::insert(php::parameters &params) {
        php::array cmd(8);
        cmd.set("insert", name_);
        php::array docs = params[0];
        if (!docs.exists(0)) { // 单项插入的情况
            docs = php::array(2);
            docs.set(0, params[0]);
        }
        cmd.set("documents", docs);
        if (params.size() > 1) cmd.set("ordered", params[1].to_boolean());

        coroutine_handler ch {coroutine::current};
        auto conn_ = cp_->acquire(ch);
        return cp_->exec(conn_, cmd, _connection_base::COMMAND_WRITE, ch);
    }

    php::value collection::delete_(php::parameters &params) {
        php::array cmd(8);
        cmd.set("delete", name_);
        php::array deletes(2), deleteo(2);
        deleteo.set("q", params[0]);
        if (params.size() > 1 && params[1].typeof(php::TYPE::INTEGER))
            deleteo.set("limit", params[1].to_integer());
        else deleteo.set("limit", 0);
        
        deletes.set(0, deleteo);
        cmd.set("deletes", deletes);
        coroutine_handler ch{coroutine::current};
        auto conn_ = cp_->acquire(ch);
        return cp_->exec(conn_, cmd, _connection_base::COMMAND_WRITE, ch);
    }

    php::value collection::delete_one(php::parameters& params) {
        return call("delete", {params[0], 1});
    }

    php::value collection::update(php::parameters &params) {
        php::array cmd(8);
        cmd.set("update", name_);
        php::array updates(2), updateo(4);
        updateo.set("q", params[0]);
        updateo.set("u", params[1]);
        if (params.size() > 2)
            updateo.set("upsert", params[2].to_boolean());

        updateo.set("multi", true);
        updates.set(0, updateo);
        cmd.set("updates", updates);
        coroutine_handler ch{coroutine::current};
        auto conn_ = cp_->acquire(ch);
        return cp_->exec(conn_, cmd, _connection_base::COMMAND_WRITE, ch);
    }

    php::value collection::update_one(php::parameters &params) {
        php::array cmd(8);
        cmd.set("update", name_);
        php::array updates(2), updateo(4);
        updateo.set("q", params[0]);
        updateo.set("u", params[1]);
        if (params.size() > 2)
            updateo.set("upsert", params[2].to_boolean());

        // updateo.set("multi", false);
        updates.set(0, updateo);
        cmd.set("updates", updates);
        coroutine_handler ch{coroutine::current};
        auto conn_ = cp_->acquire(ch);
        return cp_->exec(conn_, cmd, _connection_base::COMMAND_WRITE, ch);
    }

    php::value collection::find(php::parameters &params) {
        php::array cmd(8);
        cmd.set("find", name_);
        if (params.size() > 0)
            cmd.set("filter", params[0]);

        if (params.size() > 1) {
            php::array project;
            if (params[1].typeof(php::TYPE::ARRAY)) {
                php::array fields = params[1];
                if (fields.exists(0)) { // 将纯字段列表形式转换为K/V形式
                    project = php::array(8);
                    for (auto i = fields.begin(); i != fields.end(); ++i)
                        project.set(php::string(i->second), 1);
                } else project = fields;
            } else if (params[1].typeof(php::TYPE::STRING)) { // 单个字段
                php::array project(2);
                project.set(php::string(params[1]), 1);
            } else goto PROJECTION_ALL;

            if (!project.exists("_id")) project.set("_id", 0);
            cmd.set("projection", project);
        }
    PROJECTION_ALL:
        if (params.size() > 2 && params[2].typeof(php::TYPE::ARRAY))
            cmd.set("sort", params[2]);
        if (params.size() > 3) {
            if (params[3].typeof(php::TYPE::INTEGER))
                cmd.set("limit", params[3]);
            else if (params[3].typeof(php::TYPE::ARRAY)) {
                php::array limits = params[3];
                if (limits.size() > 0) cmd.set("skip", limits[0].to_integer());
                if (limits.size() > 1) cmd.set("limit", limits[1].to_integer());
            }
        }
        coroutine_handler ch {coroutine::current};
        auto conn_ = cp_->acquire(ch);
        return cp_->exec(conn_, cmd, _connection_base::COMMAND_READ, ch);
    }

    php::value collection::find_one(php::parameters &params) {
        php::array cmd(8);
        cmd.set("find", name_);
        cmd.set("filter", params[0]);
        if (params.size() > 1 && params[1].typeof(php::TYPE::ARRAY))
            cmd.set("sort", params[1]);

        cmd.set("limit", 1);
        coroutine_handler ch{coroutine::current};
        auto conn_ = cp_->acquire(ch);
        php::object cs = cp_->exec(conn_, cmd, _connection_base::COMMAND_READ, ch);
        assert(cs.instanceof(php::class_entry<cursor>::entry()));
        return cs.call("fetch_row");
    }
    php::value collection::get(php::parameters &params)
    {
        php::array cmd(8);
        cmd.set("find", name_);
        php::array project(2);
        php::string field = params[1];
        project.set(field, 1);
        cmd.set("projection", project);
        cmd.set("filter", params[0]);
        if (params.size() > 2 && params[2].typeof(php::TYPE::ARRAY))
            cmd.set("sort", params[2]);
        cmd.set("limit", 1);

        coroutine_handler ch{coroutine::current};
        auto conn_ = cp_->acquire(ch);
        php::object cs = cp_->exec(conn_, cmd, _connection_base::COMMAND_READ, ch);
        assert(cs.instanceof (php::class_entry<cursor>::entry()));
        php::array row = cs.call("fetch_row");
        if (row.empty()) return nullptr;
        else return row.get(field);
    }

    php::value collection::count(php::parameters &params) {
        php::array cmd(8);
        cmd.set("count", name_);
        cmd.set("query", params[0]);

        coroutine_handler ch{coroutine::current};
        auto conn_ = cp_->acquire(ch);
        php::array cs = cp_->exec(conn_, cmd, _connection_base::COMMAND_READ, ch);
        return cs.get("n");
    }

    php::value collection::aggregate(php::parameters &params) {
        php::array cmd(8);
        cmd.set("aggregate", name_);
        php::array pipeline = params[0];
        if (!pipeline.exists(0)) throw php::exception(zend_ce_type_error
            , "Failed to aggregate: 'pipeline' typeof array required"
            , -1);
        cmd.set("pipeline", pipeline);
        cmd.set("cursor", php::object(php::CLASS(zend_standard_class_def)));
        if(params.size() > 1) {
            php::array options = params[1];
            for(auto i=options.begin(); i!=options.end(); ++i)
                cmd.set(i->first.to_string(), i->second);
        }
        int execute_type = _connection_base::COMMAND_READ;
        if(params.size() > 2)
            execute_type = params[2].to_integer();
        
        coroutine_handler ch {coroutine::current};
        auto conn_ = cp_->acquire(ch);
        return cp_->exec(conn_, cmd, execute_type, ch);
    }

    php::value collection::find_and_delete(php::parameters& params) {
        php::array cmd(8);
        cmd.set("query", params[0]);
        cmd.set("remove", true);
        if(params.size() > 1) {
            php::value f = params[1];
            if(f.typeof(php::TYPE::ARRAY)) cmd.set("sort", f);
        }
        if(params.size() > 2) cmd.set("upsert", params[2].to_boolean());
        if(params.size() > 3) {
            php::value f = params[3];
            if(f.typeof(php::TYPE::ARRAY)) cmd.set("fields", f);
        }
        coroutine_handler ch {coroutine::current};
        auto conn_ = cp_->acquire(ch);
        return cp_->exec(conn_, cmd, _connection_base::COMMAND_READ_WRITE, ch);
    }

    php::value collection::find_and_update(php::parameters& params) {
        php::array cmd(8);
        cmd.set("query", params[0]);
        if(params.size() > 1) {
            php::value f = params[1];
            if(f.typeof(php::TYPE::ARRAY)) cmd.set("update", f);
        }
        if(params.size() > 2) {
            php::value f = params[2];
            if(f.typeof(php::TYPE::ARRAY)) cmd.set("sort", f);
        }
        if(params.size() > 3)
            cmd.set("upsert", params[3].to_boolean());
        if(params.size() > 4) {
            php::value f = params[4];
            if(f.typeof(php::TYPE::ARRAY)) cmd.set("fields", f);
        }
        if(params.size() > 5)
            cmd.set("new", params[5].to_boolean());
        
        coroutine_handler ch {coroutine::current};
        auto conn_ = cp_->acquire(ch);
        return cp_->exec(conn_, cmd, _connection_base::COMMAND_READ_WRITE, ch);
    }
} // namespace flame::mongodb
