#include "../coroutine.h"
#include "_command_base.h"
#include "_transaction.h"
#include "_command.h"
#include "_client.h"
#include "transaction.h"

namespace flame {
namespace redis {
	void transaction::declare(php::extension_entry& ext) {
		php::class_entry<transaction> class_transaction("flame\\redis\\transaction");
		class_transaction
			.method<&transaction::__construct>("__construct", {}, php::PRIVATE)
			.method<&transaction::__destruct>("__destruct")
			.method<&transaction::__call>("__call", {
				{"cmd", php::TYPE::STRING},
				{"arg", php::TYPE::ARRAY},
			})
			.method<&transaction::mget>("mget", {
				{"key", php::TYPE::STRING},
			})
			.method<&transaction::hmget>("hmget", {
				{"hash", php::TYPE::STRING},
			})
			.method<&transaction::hgetall>("hgetall", {
				{"hash", php::TYPE::STRING},
			})
			.method<&transaction::hscan>("hscan", {
				{"hash", php::TYPE::STRING},
				{"cursor", php::TYPE::INTEGER},
			})
			.method<&transaction::zscan>("zscan", {
				{"zset", php::TYPE::STRING},
				{"cursor", php::TYPE::INTEGER},
			})
			.method<&transaction::zrange>("zrange", {
				{"zset", php::TYPE::STRING},
				{"start", php::TYPE::INTEGER},
				{"stop", php::TYPE::INTEGER},
			})
			.method<&transaction::zrevrange>("zrevrange", {
				{"zset", php::TYPE::STRING},
				{"start", php::TYPE::INTEGER},
				{"stop", php::TYPE::INTEGER},
			})
			.method<&transaction::zrangebyscore>("zrangebyscore", {
				{"zset", php::TYPE::STRING},
				{"min", php::TYPE::STRING},
				{"max", php::TYPE::STRING},
			})
			.method<&transaction::zrevrangebyscore>("zrevrangebyscore", {
				{"zset", php::TYPE::STRING},
				{"min", php::TYPE::STRING},
				{"max", php::TYPE::STRING},
			})
			.method<&transaction::unsubscribe>("unsubscribe")
			.method<&transaction::punsubscribe>("punsubscribe")
			.method<&transaction::exec>("exec")
			// 部分命令在 transaction 不能使用
			.method<&transaction::unsupport>("subscribe")
			.method<&transaction::unsupport>("psubscribe")
			.method<&transaction::unsupport>("multi")
			.method<&transaction::unsupport>("pipel");
		ext.add(std::move(class_transaction));
	}
	php::value transaction::__construct(php::parameters& params) {
		return nullptr;
	}
	php::value transaction::__destruct(php::parameters& params) {
		if(tx_ != nullptr) delete tx_;
		return nullptr;
	}
	php::value transaction::__call(php::parameters& params) {
		tx_->append(new _command(params[0], php::array(params[1])));
		return php::value(this);
	}
	php::value transaction::mget(php::parameters& params) {
		tx_->append(new _command("MGET", params, _command_base::REPLY_COMBINE_1));
		return php::value(this);
	}
	php::value transaction::hmget(php::parameters& params) {
		tx_->append(new _command("HMGET", params, _command_base::REPLY_COMBINE_2));
		return php::value(this);
	}
	php::value transaction::hgetall(php::parameters& params) {
		tx_->append(new _command("HGETALL", params, _command_base::REPLY_ASSOC_ARRAY_1));
		return php::value(this);
	}
	php::value transaction::hscan(php::parameters& params) {
		tx_->append(new _command("HSCAN", params, _command_base::REPLY_ASSOC_ARRAY_2));
		return php::value(this);
	}
	php::value transaction::zscan(php::parameters& params) {
		tx_->append(new _command("ZSCAN", params, _command_base::REPLY_ASSOC_ARRAY_2));
		return php::value(this);
	}
	php::value transaction::zrange(php::parameters& params) {
		php::string larg = params[params.size() - 1];
		if(larg.typeof(php::TYPE::STRING) && larg.size() == 10 && strncasecmp("WITHSCORES", larg.c_str(), 10) == 0) {
			tx_->append(new _command("ZRANGE", params, _command_base::REPLY_ASSOC_ARRAY_1));
		}else{
			tx_->append(new _command("ZRANGE", params));
		}
		return php::value(this);
	} 
	php::value transaction::zrevrange(php::parameters& params) {
		php::string arg = params[params.size() - 1];
		if(arg.typeof(php::TYPE::STRING) && arg.size() == 10 && strncasecmp("WITHSCORES", arg.c_str(), 10) == 0) {
			tx_->append(new _command("ZREVRANGE", params, _command_base::REPLY_ASSOC_ARRAY_1));
		}else{
			tx_->append(new _command("ZREVRANGE", params));
		}
		return php::value(this);
	}
	php::value transaction::zrangebyscore(php::parameters& params) {
		for(int i=3; i<params.size(); ++i) {
			if(params[i].typeof(php::TYPE::STRING) && params[i].size() == 10) {
				php::string arg = params[i];
				if(strncasecmp("WITHSCORES", arg.c_str(), 10) == 0) {
					tx_->append(new _command("ZRANGEBYSCORE", params, _command_base::REPLY_ASSOC_ARRAY_1));
					return php::value(this);
				}
			}
		}
		tx_->append(new _command("ZRANGEBYSCORE", params));
		return php::value(this);
	}
	php::value transaction::zrevrangebyscore(php::parameters& params) {
		for(int i=3; i<params.size(); ++i) {
			if(params[i].typeof(php::TYPE::STRING) && params[i].size() == 10) {
				php::string arg = params[i];
				if(strncasecmp("WITHSCORES", arg.c_str(), 10) == 0) {
					tx_->append(new _command("ZREVRANGEBYSCORE", params, _command_base::REPLY_ASSOC_ARRAY_1));
					return php::value(this);
				}
			}
		}
		tx_->append(new _command("ZREVRANGEBYSCORE", params));
		return php::value(this);
	}
	php::value transaction::unsubscribe(php::parameters& params) {
		tx_->append(new _command("ZREVRANGEBYSCORE", params, _command_base::REPLY_ASSOC_ARRAY_1));
		return php::value(this);
	}
	php::value transaction::punsubscribe(php::parameters& params) {
		tx_->append(new _command("ZREVRANGEBYSCORE", params, _command_base::REPLY_ASSOC_ARRAY_1));
		return php::value(this);
	}
	php::value transaction::exec(php::parameters& params) {
		cli_->exec(coroutine::current, tx_);
		tx_ = nullptr;
		return coroutine::async();
	}
	php::value transaction::unsupport(php::parameters& params) {
		throw php::exception(zend_ce_error, "not supported in transaction/pipeline");
	}
}
}
