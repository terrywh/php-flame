#include "../coroutine.h"
#include "tx.h"
#include "_connection_lock.h"

namespace flame::redis {
	void tx::declare(php::extension_entry& ext) {
		php::class_entry<tx> class_tx("flame\\redis\\tx");
		class_tx
			.method<&tx::__construct>("__construct", {}, php::PRIVATE)
            .method<&tx::exec>("exec")
			.method<&tx::__call>("__call",
            {
				{"cmd", php::TYPE::STRING},
				{"arg", php::TYPE::ARRAY},
			})
			.method<&tx::mget>("mget",
            {
				{"key", php::TYPE::STRING},
			})
			.method<&tx::hmget>("hmget",
            {
				{"hash", php::TYPE::STRING},
			})
			.method<&tx::hgetall>("hgetall",
            {
				{"hash", php::TYPE::STRING},
			})
			.method<&tx::hscan>("hscan",
            {
				{"hash", php::TYPE::STRING},
				{"cursor", php::TYPE::INTEGER},
			})
			.method<&tx::zscan>("zscan",
            {
				{"zset", php::TYPE::STRING},
				{"cursor", php::TYPE::INTEGER},
			})
			.method<&tx::zrange>("zrange",
            {
				{"zset", php::TYPE::STRING},
				{"start", php::TYPE::INTEGER},
				{"stop", php::TYPE::INTEGER},
			})
			.method<&tx::zrevrange>("zrevrange",
            {
				{"zset", php::TYPE::STRING},
				{"start", php::TYPE::INTEGER},
				{"stop", php::TYPE::INTEGER},
			})
			.method<&tx::zrangebyscore>("zrangebyscore",
            {
				{"zset", php::TYPE::STRING},
				{"min", php::TYPE::STRING},
				{"max", php::TYPE::STRING},
			})
			.method<&tx::zrevrangebyscore>("zrevrangebyscore",
            {
				{"zset", php::TYPE::STRING},
				{"min", php::TYPE::STRING},
				{"max", php::TYPE::STRING},
			})
			.method<&tx::unimplement>("multi")
			// 以下暂未实现
			.method<&tx::unimplement>("subscribe")
			.method<&tx::unimplement>("psubscribe")
            .method<&tx::unimplement>("unsubscribe")
			.method<&tx::unimplement>("punsubscribe");
        
		ext.add(std::move(class_tx));
	}

	php::value tx::__construct(php::parameters& params)
	{
		return nullptr;
	}
    php::value tx::exec(php::parameters& params)
    {
        coroutine_handler ch {coroutine::current};
        return cl_->exec(ch);
    }
	php::value tx::__call(php::parameters& params)
	{
        php::string name = params[0];
        php::array  argv = params[1];
        cl_->push(name, argv, reply_type::SIMPLE);
        return this;
	}
	php::value tx::mget(php::parameters& params)
	{
		php::string name("MGET", 4);
		cl_->push(name, params, reply_type::COMBINE_1);
        return this;
    }
	php::value tx::hmget(php::parameters& params)
	{
		php::string name("HMGET", 5);
        cl_->push(name, params, reply_type::COMBINE_2);
        return this;
    }
	php::value tx::hgetall(php::parameters& params)
	{
		php::string name("HGETALL", 7);
        cl_->push(name, params, reply_type::ASSOC_ARRAY_1);
        return this;
    }
	php::value tx::hscan(php::parameters& params) {
		php::string name("HSCAN", 5);
		cl_->push(name, params, reply_type::ASSOC_ARRAY_2);
        return this;
    }
	php::value tx::zscan(php::parameters& params) {
		php::string name("ZSCAN", 5);
        cl_->push(name, params, reply_type::ASSOC_ARRAY_2);
        return this;
    }
	php::value tx::zrange(php::parameters& params) {
		php::string name("ZRANGE", 6);
		php::string last = params[params.size()-1];
		if (last.typeof(php::TYPE::STRING) && last.size() == 10 && strncasecmp("WITHSCORES", last.c_str(), 10) == 0)
		{
            cl_->push(name, params, reply_type::ASSOC_ARRAY_1);
        }
		else
		{
            cl_->push(name, params, reply_type::SIMPLE);
        }
        return this;
    } 
	php::value tx::zrevrange(php::parameters& params) {
		php::string name("ZREVRANGE", 9);
		php::string last = params[params.size() - 1];
		if (last.typeof(php::TYPE::STRING) &&last.size() == 10 && strncasecmp("WITHSCORES", last.c_str(), 10) == 0)
		{
            cl_->push(name, params, reply_type::ASSOC_ARRAY_1);
        }
		else
		{
            cl_->push(name, params, reply_type::SIMPLE);
        }
        return this;
    }
	php::value tx::zrangebyscore(php::parameters& params) {
		php::string name("ZRANGEBYSCORE", 13);

		for(int i=3; i<params.size(); ++i) {
			if(params[i].typeof(php::TYPE::STRING) && params[i].size() == 10) {
				php::string arg = params[i];
				if(strncasecmp("WITHSCORES", arg.c_str(), 10) == 0) {
                    cl_->push(name, params, reply_type::ASSOC_ARRAY_1);
                    return this;
                }
			}
		}
        cl_->push(name, params, reply_type::SIMPLE);
        return this;
    }
	php::value tx::zrevrangebyscore(php::parameters& params) {
		php::string name("ZREVRANGEBYSCORE", 16);

		for(int i=3; i<params.size(); ++i) {
			if(params[i].typeof(php::TYPE::STRING) && params[i].size() == 10) {
				php::string arg = params[i];
				if(strncasecmp("WITHSCORES", arg.c_str(), 10) == 0) {
                    cl_->push(name, params, reply_type::ASSOC_ARRAY_1);
                    return this;
                }
			}
		}
        cl_->push(name, params, reply_type::SIMPLE);
        return this;
    }
	php::value tx::unimplement(php::parameters& params) {
        throw php::exception(zend_ce_type_error, "This redis command is NOT yet implemented");
    }
} // namespace flame::redis
