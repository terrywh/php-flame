#include "redis.h"
//#define MY_DEBUG

namespace flame {
namespace db {


void go_pause() {
	uv_stop(uv_default_loop());
}

void set_redis_array_element(php::value& elem, redisReply* reply) {
	switch(reply->type)
	{
	case REDIS_REPLY_INTEGER:
		{
			elem = (std::int64_t)reply->integer;
		}
		break;
	case REDIS_REPLY_STRING:
		{
			elem = php::string(reply->str);
		}
		break;
	case REDIS_REPLY_ERROR:
		{
			// array里藏着一个error？不能吧
			assert(0);
			elem = php::string();
		}
		break;
	case REDIS_REPLY_ARRAY:
		{
			// array嵌套，比较危险，应该没有吧？
			php::array arr(reply->elements);
			for(int i = 0; i < reply->elements; ++i) {
				set_redis_array_element(arr[i], reply->element[i]);
			}
		}
		break;
	case REDIS_REPLY_NIL:
		{
			elem = php::value(nullptr);
		}
		break;
	}
}

void redis::default_set_result(void *r, void *privdata) {
	redisReply *reply = (redisReply*)r;
	if (reply == NULL) {
		set_result(php::string());
		set_error(php::string());
	} else {
		set_error(php::string());
		switch(reply->type)
		{
		case REDIS_REPLY_INTEGER:
			{
				set_result((std::int64_t)reply->integer);
			}
			break;
		case REDIS_REPLY_STRING:
			{
				set_result(php::string(reply->str));
			}
			break;
		case REDIS_REPLY_ERROR:
			{
				set_result(php::string());
				set_error(php::string(reply->str));
			}
			break;
		case REDIS_REPLY_ARRAY:
			{
				php::array arr(reply->elements);
				for(int i = 0; i < reply->elements; ++i) {
					set_redis_array_element(arr[i], reply->element[i]);
				}
				set_result(arr);
			}
			break;
		case REDIS_REPLY_NIL:
			{
				set_result(php::value(nullptr));
			}
			break;
		case REDIS_REPLY_STATUS:
			{
				set_result(php::string(reply->str));
			}
			break;
		default:
			{
				// 未知类型
				assert(0);
			}
		}
	}
}

void redis::default_callback(redisAsyncContext *c, void *r, void *privdata) {
	redis* pClient = (redis*)c->data;
	pClient->default_set_result(r, privdata);
	flame::fiber*  f = pClient->_fiber;
	f->next(pClient->result_);
}

void redis::return_pair_callback(redisAsyncContext *c, void *r, void *privdata) {
	redisReply *reply = (redisReply*)r;
	redis* pClient = (redis*)privdata;
	if (reply == NULL || (reply->type == REDIS_REPLY_NIL)) {
		pClient->set_result(php::string());
		pClient->set_error(php::string());
	} else {
		pClient->set_error(php::string());
		if (reply->type == REDIS_REPLY_ARRAY) {
			// i是key，i+1就是value
			php::array arr(reply->elements/2);
			for(int i = 0; i < reply->elements; i=i+2) {
				const char* key = reply->element[i]->str;
				set_redis_array_element(arr[key], reply->element[i+1]);
			}
			pClient->set_result(arr);
		} else {
			//hgetall不返回array？
		}
	}
	flame::fiber*  f = pClient->_fiber;
	f->next(pClient->result_);
}

void redis::arg_key_callback(redisAsyncContext *c, void *r, void *privdata) {
	redisReply *reply = (redisReply*)r;
	redis_cmd_cache* pCache = (redis_cmd_cache*)privdata;
	redis* pClient = (redis*)c->data;
	if (reply == NULL || (reply->type == REDIS_REPLY_NIL)) {
		pClient->set_result(php::string());
		pClient->set_error(php::string());
	} else {
		pClient->set_error(php::string());
		if (reply->type == REDIS_REPLY_ARRAY) {
			// key在Cache里取
			php::array arr(reply->elements);
			for(int i = 0; i < reply->elements; ++i) {
				// 第一个是cmd，第二个是hash名，所以要加2
				const char* key = pCache->data[i+2].data();
				set_redis_array_element(arr[key], reply->element[i]);
			}
			pClient->set_result(arr);
		} else {
			//hmget不返回array？
		}
	}
	flame::fiber*  f = pClient->_fiber;
	f->next(pClient->result_);
}

void redis::quit_callback(redisAsyncContext *c, void *r, void *privdata) {
	redis* pClient = (redis*)privdata;
	default_callback(c, r, privdata);
	php::string& result = pClient->result_;
	pClient->close();
	flame::fiber*  f = pClient->_fiber;
	f->next(result);
	
}

void connect_callback(const redisAsyncContext *c, int status) {
	if (status != REDIS_OK) {
		printf("Error: %s\n", c->errstr);
		return;
	}
#ifdef MY_DEBUG
	printf("Redis connected...\n");
#endif
}

void disconnect_callback(const redisAsyncContext *c, int status) {
	redis* pClient = (redis*)c->data;
	pClient->set_context(nullptr);
	if (status != REDIS_OK) {
		printf("Error: %s\n", c->errstr);
		return;
	}
#ifdef MY_DEBUG
	printf("Disconnected...\n");
#endif
}

redis::redis()
: context_(nullptr) {
}

redis::~redis() {
	close();
}


void redis::connect(php::array& arr) {
	if (context_) {
		// 已经连接过一次了？
		close();
	}
	php::string& host = arr["host"];
	int port = arr["port"];
	context_ = redisAsyncConnect(host.c_str(), port);
	if (context_->err) {
		set_error(context_->err);
		return;
	}
	context_->data = this;
	uv_loop_t* loop = uv_default_loop();
	redisLibuvAttach(context_,loop);
	redisAsyncSetConnectCallback(context_,connect_callback);
	redisAsyncSetDisconnectCallback(context_,disconnect_callback);
	php::value* pauth = arr.find("auth");
	if (pauth) {
		php::string auth = arr["auth"];
		command("AUTH", auth.c_str());
	}
	php::value* pselect = arr.find("select");
	if (pselect) {
		std::string db = arr["select"].to_string();
		command("SELECT", db.c_str());
	}

}

php::value redis::__construct(php::parameters& params) {
	if (params.length() > 0) {
		php::array& arr = params[0];
		error_ = php::string();
		connect(arr);
	}
	return this;
}


php::value redis::__call(php::parameters& params) {
	php::string& cmd = params[0];
	if (params.length() > 1) {
		php::array& args = params[1];
		int argc = args.length() + 1;
		std::vector<char*> argv;
		std::vector<std::string> strs;
		strs.reserve(argc);
		std::vector<size_t> argvlen;
		// 先放cmd
		strs.push_back(cmd.c_str());
		argv.push_back((char*)strs[0].data());
		// 再放参数
		for(int i = 1; i < argc; i++) {
			php::string arg = args[i-1].to_string();
			strs.push_back(arg.c_str());
			argv.push_back((char*)strs[i].data());
		}
		if (strcasecmp(strs.back().c_str(),"WITHSCORES") == 0) {
			// 如果带withscores，就换一下default函数
			command(argc, (const char**)argv.data(), NULL, return_pair_callback);
		} else {
			command(argc, (const char**)argv.data(), NULL);
		}
	} else {
		command(cmd.c_str());
	}
	return flame::async;
}


php::value redis::connect(php::parameters& params) {
	php::array& arr = params[0];
	connect(arr);
	return nullptr;
}

php::value redis::close(php::parameters& params) {
	close();
	return nullptr;
}

void redis::close() {
	if (context_) {
		redisAsyncDisconnect(context_);
		context_ = nullptr;
		result_ = php::string();
	}
}

php::value redis::hgetall(php::parameters& params) {
	php::string& hash = params[0];
	command("HGETALL", hash.c_str(), return_pair_callback);
	return flame::async;
}

void redis::command_arg_key(const char* cmd, php::parameters& params) {
	cache_.data.clear();
	// 预先申请好内存，后面好取到正确的指针，否则vector空间不够重新分配argv就废掉了
	cache_.data.reserve(params.length()+1);
	cache_.data.push_back(cmd);

	std::vector<const char*> argv;
	// 先放cmd
	argv.push_back(cache_.data[0].data());
	// 再放参数
	for( int i=0; i < params.length(); ++i) {
		std::string arg = params[i].to_string();
		cache_.data.push_back(arg);
		argv.push_back(cache_.data[i+1].data());
	}
	command(cache_.data.size(), (const char**)argv.data(), NULL, arg_key_callback, &cache_);
}

php::value redis::hmget(php::parameters& params) {
	command_arg_key("HMGET", params);
	return flame::async;
}


void redis::subscribe_callback(redisAsyncContext *c, void *r, void *privdata) {
	redis* pClient = (redis*)c->data;
	pClient->default_set_result(r, privdata);
	php::callable cb = *(php::callable*)privdata;
	php::value& result = pClient->GetResult();
	if (result.is_null()) {
		// 说明报错了
	} else {
		php::array& arr = result;
		cb(pClient, arr[1], arr[2]);
	}
}

php::value redis::subscribe(php::parameters& params) {
	php::array& channel = params[0];
	php::callable& cb = params[1];
	std::vector<std::string> data;
	// 预先申请好内存，后面好取到正确的指针
	data.reserve(params.length()+1);
	data.push_back("SUBSCRIBE");

	std::vector<const char*> argv;
	// 先放cmd
	argv.push_back(data[0].data());
	// 再放参数
	int argv_index = 1;
	for( auto iter = channel.begin(); iter != channel.end(); ++iter) {
		std::string arg = (iter->second).to_string();
		data.push_back(arg);
		argv.push_back(data[argv_index].data());
		++argv_index;
	}
	command(data.size(), (const char**)argv.data(), NULL, subscribe_callback, (void*)&cb);
	return nullptr;
}

php::value redis::quit(php::parameters& params) {
	command("QUIT", quit_callback);
	return flame::async;
}

void redis::command(const char* cmd, redisCallbackFn *fn, void *privdata) {
	if (!context_) {
		set_error("no connection");
		return;
	}
	void* priv = privdata;
	if (priv == nullptr) {
		priv = this;
	}
	_fiber = flame::this_fiber();
	redisAsyncCommand(context_, fn, priv, "%s", cmd);
}

void redis::command(const char* cmd, const char* arg, redisCallbackFn *fn, void *privdata) {
	if (!context_) {
		set_error("no connection");
		return;
	}
	void* priv = privdata;
	if (priv == nullptr) {
		priv = this;
	}
	_fiber = flame::this_fiber();
	redisAsyncCommand(context_, fn, priv, "%s %s", cmd, arg);
}

void redis::command(int argc, const char **argv, const size_t *argvlen, redisCallbackFn *fn, void *privdata) {
	if (!context_) {
		set_error("no connection");
		return;
	}
	void* priv = privdata;
	if (priv == nullptr) {
		priv = this;
	}
	_fiber = flame::this_fiber();
	redisAsyncCommandArgv(context_, fn, priv, argc, argv, argvlen);
}

}
}

