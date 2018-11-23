#include "../coroutine.h"
#include "client.h"
#include "_connection_pool.h"
#include "tx.h"
#include "_connection_lock.h"

namespace flame::redis {
	void client::declare(php::extension_entry& ext) {
		php::class_entry<client> class_client("flame\\redis\\client");
		class_client
			.method<&client::__construct>("__construct", {}, php::PRIVATE)
			.method<&client::__call>("__call", {
				{"cmd", php::TYPE::STRING},
				{"arg", php::TYPE::ARRAY},
			})
			.method<&client::mget>("mget", {
				{"key", php::TYPE::STRING},
			})
			.method<&client::hmget>("hmget", {
				{"hash", php::TYPE::STRING},
			})
			.method<&client::hgetall>("hgetall", {
				{"hash", php::TYPE::STRING},
			})
			.method<&client::hscan>("hscan", {
				{"hash", php::TYPE::STRING},
				{"cursor", php::TYPE::INTEGER},
			})
			.method<&client::zscan>("zscan", {
				{"zset", php::TYPE::STRING},
				{"cursor", php::TYPE::INTEGER},
			})
			.method<&client::zrange>("zrange", {
				{"zset", php::TYPE::STRING},
				{"start", php::TYPE::INTEGER},
				{"stop", php::TYPE::INTEGER},
			})
			.method<&client::zrevrange>("zrevrange", {
				{"zset", php::TYPE::STRING},
				{"start", php::TYPE::INTEGER},
				{"stop", php::TYPE::INTEGER},
			})
			.method<&client::zrangebyscore>("zrangebyscore", {
				{"zset", php::TYPE::STRING},
				{"min", php::TYPE::STRING},
				{"max", php::TYPE::STRING},
			})
			.method<&client::zrevrangebyscore>("zrevrangebyscore", {
				{"zset", php::TYPE::STRING},
				{"min", php::TYPE::STRING},
				{"max", php::TYPE::STRING},
			})
			.method<&client::multi>("multi")
			// 以下暂未实现
			.method<&client::unimplement>("subscribe")
			.method<&client::unimplement>("psubscribe")
            .method<&client::unimplement>("unsubscribe")
			.method<&client::unimplement>("punsubscribe");
        
		ext.add(std::move(class_client));
	}

	php::value client::__construct(php::parameters& params)
	{
		return nullptr;
	}
	php::value client::__call(php::parameters& params)
	{
        coroutine_handler ch {coroutine::current};
        auto rc = cp_->acquire(ch);
        php::string name = params[0];
        php::array  argv = params[1];
        return cp_->exec(rc, name, argv, reply_type::SIMPLE, ch);
	}
	php::value client::mget(php::parameters& params)
	{
		coroutine_handler ch{coroutine::current};
		auto rc = cp_->acquire(ch);
		php::string name("MGET", 4);
		return cp_->exec(rc, name, params, reply_type::COMBINE_1, ch);
	}
	php::value client::hmget(php::parameters& params)
	{
		coroutine_handler ch{coroutine::current};
		auto rc = cp_->acquire(ch);
		php::string name("HMGET", 5);
		return cp_->exec(rc, name, params, reply_type::COMBINE_2, ch);
	}
	php::value client::hgetall(php::parameters& params)
	{
		coroutine_handler ch{coroutine::current};
		auto rc = cp_->acquire(ch);
		php::string name("HGETALL", 7);
		return cp_->exec(rc, name, params, reply_type::ASSOC_ARRAY_1, ch);
	}
	php::value client::hscan(php::parameters& params) {
		coroutine_handler ch{coroutine::current};
		auto rc = cp_->acquire(ch);
		php::string name("HSCAN", 5);
		return cp_->exec(rc, name, params, reply_type::ASSOC_ARRAY_2, ch);
	}
	php::value client::zscan(php::parameters& params) {
		coroutine_handler ch{coroutine::current};
		auto rc = cp_->acquire(ch);
		php::string name("ZSCAN", 5);
		return cp_->exec(rc, name, params, reply_type::ASSOC_ARRAY_2, ch);
	}
	php::value client::zrange(php::parameters& params) {
		coroutine_handler ch{coroutine::current};
		auto rc = cp_->acquire(ch);
		php::string name("ZRANGE", 6);
		php::string last = params[params.size()-1];
		if (last.typeof(php::TYPE::STRING) && last.size() == 10 && strncasecmp("WITHSCORES", last.c_str(), 10) == 0)
		{
			return cp_->exec(rc, name, params, reply_type::ASSOC_ARRAY_1, ch);
		}
		else
		{
			return cp_->exec(rc, name, params, reply_type::SIMPLE, ch);
		}
	} 
	php::value client::zrevrange(php::parameters& params) {
		coroutine_handler ch{coroutine::current};
		auto rc = cp_->acquire(ch);
		php::string name("ZREVRANGE", 9);
		php::string last = params[params.size() - 1];
		if (last.typeof(php::TYPE::STRING) &&last.size() == 10 && strncasecmp("WITHSCORES", last.c_str(), 10) == 0)
		{
			return cp_->exec(rc, name, params, reply_type::ASSOC_ARRAY_1, ch);
		}
		else
		{
			return cp_->exec(rc, name, params, reply_type::SIMPLE, ch);
		}
	}
	php::value client::zrangebyscore(php::parameters& params) {
		coroutine_handler ch{coroutine::current};
		auto rc = cp_->acquire(ch);
		php::string name("ZRANGEBYSCORE", 13);

		for(int i=3; i<params.size(); ++i) {
			if(params[i].typeof(php::TYPE::STRING) && params[i].size() == 10) {
				php::string arg = params[i];
				if(strncasecmp("WITHSCORES", arg.c_str(), 10) == 0) {
					return cp_->exec(rc, name, params, reply_type::ASSOC_ARRAY_1, ch);
				}
			}
		}
		return cp_->exec(rc, name, params, reply_type::SIMPLE, ch);
	}
	php::value client::zrevrangebyscore(php::parameters& params) {
		coroutine_handler ch{coroutine::current};
		auto rc = cp_->acquire(ch);
		php::string name("ZREVRANGEBYSCORE", 16);

		for(int i=3; i<params.size(); ++i) {
			if(params[i].typeof(php::TYPE::STRING) && params[i].size() == 10) {
				php::string arg = params[i];
				if(strncasecmp("WITHSCORES", arg.c_str(), 10) == 0) {
					return cp_->exec(rc, name, params, reply_type::ASSOC_ARRAY_1, ch);
				}
			}
		}
		return cp_->exec(rc, name, params, reply_type::SIMPLE, ch);
	}
	php::value client::multi(php::parameters& params) {
        coroutine_handler ch{coroutine::current};
        auto conn_ = cp_->acquire(ch);
		php::object obj(php::class_entry<tx>::entry());
		tx *ptr = static_cast<tx *>(php::native(obj));
		ptr->cl_.reset(new _connection_lock(conn_));
		return std::move(obj);
	}
	php::value client::unimplement(php::parameters& params) {
        throw php::exception(zend_ce_error, "This redis command is NOT yet implemented");
    }
} // namespace flame::redis
