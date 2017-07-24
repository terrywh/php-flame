#include <typeinfo>
#include <iostream>
#include <string.h>
#include "redis_client.h"
//#define MY_DEBUG

namespace flame {
namespace db {


void go_pause()
{
	uv_stop(uv_default_loop());
}

void SetRedisArrayElement(php::value& elem, redisReply* reply)
{
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
				SetRedisArrayElement(arr[i], reply->element[i]);
			}
		}
		break;
	case REDIS_REPLY_NIL:
		{
			elem = php::string();
		}
		break;
	}
}

void redis_client::DefaultSetResult(void *r, void *privdata)
{
	redisReply *reply = (redisReply*)r;
	if (reply == NULL) {
		SetResult(php::string());
		SetError(php::string());
	} else {
		SetError(php::string());
		switch(reply->type)
		{
		case REDIS_REPLY_INTEGER:
			{
				SetResult((std::int64_t)reply->integer);
			}
			break;
		case REDIS_REPLY_STRING:
			{
				SetResult(php::string(reply->str));
			}
			break;
		case REDIS_REPLY_ERROR:
			{
				SetResult(php::string());
				SetError(php::string(reply->str));
			}
			break;
		case REDIS_REPLY_ARRAY:
			{
				php::array arr(reply->elements);
				for(int i = 0; i < reply->elements; ++i) {
					SetRedisArrayElement(arr[i], reply->element[i]);
				}
				SetResult(arr);
			}
			break;
		case REDIS_REPLY_NIL:
			{
				SetResult(php::string());
			}
			break;
		case REDIS_REPLY_STATUS:
			{
				SetResult(php::string(reply->str));
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

void redis_client::DefaultCallback(redisAsyncContext *c, void *r, void *privdata)
{
	redis_client* pClient = (redis_client*)c->data;
	pClient->DefaultSetResult(r, privdata);
	flame::fiber*  f = pClient->_fiber;
	f->next(pClient->_result);
}

void redis_client::ReturnPairCallback(redisAsyncContext *c, void *r, void *privdata)
{
	redisReply *reply = (redisReply*)r;
	redis_client* pClient = (redis_client*)privdata;
	if (reply == NULL || (reply->type == REDIS_REPLY_NIL)) {
		pClient->SetResult(php::string());
		pClient->SetError(php::string());
	} else {
		pClient->SetError(php::string());
		if (reply->type == REDIS_REPLY_ARRAY) {
			// i是key，i+1就是value
			php::array arr(reply->elements/2);
			for(int i = 0; i < reply->elements; i=i+2) {
				const char* key = reply->element[i]->str;
				SetRedisArrayElement(arr[key], reply->element[i+1]);
			}
			pClient->SetResult(arr);
		} else {
			//hgetall不返回array？
		}
	}
	flame::fiber*  f = pClient->_fiber;
	f->next(pClient->_result);
}

void redis_client::ArgKeyCallback(redisAsyncContext *c, void *r, void *privdata)
{
	redisReply *reply = (redisReply*)r;
	RedisCmdCache* pCache = (RedisCmdCache*)privdata;
	redis_client* pClient = (redis_client*)c->data;
	if (reply == NULL || (reply->type == REDIS_REPLY_NIL)) {
		pClient->SetResult(php::string());
		pClient->SetError(php::string());
	} else {
		pClient->SetError(php::string());
		if (reply->type == REDIS_REPLY_ARRAY) {
			// key在Cache里取
			php::array arr(reply->elements);
			for(int i = 0; i < reply->elements; ++i) {
				// 第一个是cmd，第二个是hash名，所以要加2
				const char* key = pCache->data[i+2].data();
				SetRedisArrayElement(arr[key], reply->element[i]);
			}
			pClient->SetResult(arr);
		} else {
			//hmget不返回array？
		}
	}
	flame::fiber*  f = pClient->_fiber;
	f->next(pClient->_result);
}

void redis_client::QUITCallback(redisAsyncContext *c, void *r, void *privdata)
{
	redis_client* pClient = (redis_client*)privdata;
	DefaultCallback(c, r, privdata);
	php::string& result = pClient->_result;
	pClient->Close();
	flame::fiber*  f = pClient->_fiber;
	f->next(result);
	
}

void connectCallback(const redisAsyncContext *c, int status)
{
	if (status != REDIS_OK) {
		printf("Error: %s\n", c->errstr);
		return;
	}
#ifdef MY_DEBUG
	printf("Redis Connected...\n");
#endif
}

void disconnectCallback(const redisAsyncContext *c, int status) {
	redis_client* pClient = (redis_client*)c->data;
	pClient->SetContext(nullptr);
	if (status != REDIS_OK) {
		printf("Error: %s\n", c->errstr);
		return;
	}
#ifdef MY_DEBUG
	printf("Disconnected...\n");
#endif
}

redis_client::redis_client()
: _context(nullptr)
{
}

redis_client::~redis_client()
{
	Close();
}


void redis_client::Connect(php::array& arr)
{
	if (_context) {
		// 已经连接过一次了？
		Close();
	}
	php::string& host = arr["host"];
	int port = arr["port"];
	_context = redisAsyncConnect(host.c_str(), port);
	if (_context->err) {
		SetError(_context->err);
		return;
	}
	_context->data = this;
	uv_loop_t* loop = uv_default_loop();
	redisLibuvAttach(_context,loop);
	redisAsyncSetConnectCallback(_context,connectCallback);
	redisAsyncSetDisconnectCallback(_context,disconnectCallback);
	php::value* pauth = arr.find("auth");
	if (pauth) {
		php::string auth = arr["auth"];
		Command("AUTH", auth.c_str());
	}
	php::value* pselect = arr.find("select");
	if (pselect) {
		std::string db = arr["select"].to_string();
		Command("SELECT", db.c_str());
	}

}

php::value redis_client::__construct(php::parameters& params)
{
	if (params.length() > 0) {
		php::array& arr = params[0];
		_error = php::string();
		Connect(arr);
	}
	return this;
}


php::value redis_client::__call(php::parameters& params)
{
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
			Command(argc, (const char**)argv.data(), NULL, ReturnPairCallback);
		} else {
			Command(argc, (const char**)argv.data(), NULL);
		}
	} else {
		Command(cmd.c_str());
	}
	return flame::async;
}


php::value redis_client::connect(php::parameters& params)
{
	php::array& arr = params[0];
	Connect(arr);
	return nullptr;
}

php::value redis_client::close(php::parameters& params)
{
	Close();
	return nullptr;
}

void redis_client::Close()
{
	if (_context) {
		redisAsyncDisconnect(_context);
		_context = nullptr;
		_result = php::string();
	}
}

php::value redis_client::hgetall(php::parameters& params)
{
	php::string& hash = params[0];
	Command("HGETALL", hash.c_str(), ReturnPairCallback);
	return flame::async;
}

void redis_client::CommandArgKey(const char* cmd, php::parameters& params)
{
	_cache.data.clear();
	// 预先申请好内存，后面好取到正确的指针，否则vector空间不够重新分配argv就废掉了
	_cache.data.reserve(params.length()+1);
	_cache.data.push_back(cmd);

	std::vector<const char*> argv;
	// 先放cmd
	argv.push_back(_cache.data[0].data());
	// 再放参数
	for( int i=0; i < params.length(); ++i) {
		std::string arg = params[i].to_string();
		_cache.data.push_back(arg);
		argv.push_back(_cache.data[i+1].data());
	}
	Command(_cache.data.size(), (const char**)argv.data(), NULL, ArgKeyCallback, &_cache);
}

php::value redis_client::hmget(php::parameters& params)
{
	CommandArgKey("HMGET", params);
	return flame::async;
}


void redis_client::SUBSCRIBECallback(redisAsyncContext *c, void *r, void *privdata)
{
	redis_client* pClient = (redis_client*)c->data;
	pClient->DefaultSetResult(r, privdata);
	php::callable cb = *(php::callable*)privdata;
	php::value& result = pClient->GetResult();
	if (result.is_null()) {
		// 说明报错了
	} else {
		php::array& arr = result;
		cb(pClient, arr[1], arr[2]);
	}
}

php::value redis_client::subscribe(php::parameters& params)
{
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
	Command(data.size(), (const char**)argv.data(), NULL, SUBSCRIBECallback, (void*)&cb);
	return nullptr;
}

php::value redis_client::quit(php::parameters& params)
{
	Command("QUIT", QUITCallback);
	return flame::async;
}

void redis_client::Command(const char* cmd, redisCallbackFn *fn, void *privdata)
{
	if (!_context) {
		SetError("no connection");
		return;
	}
	void* priv = privdata;
	if (priv == nullptr) {
		priv = this;
	}
	if (flame::this_fiber() == NULL) {
		throw php::exception("need yield!", -1);
	} else {
		_fiber = flame::this_fiber();
	}
	redisAsyncCommand(_context, fn, priv, "%s", cmd);
}

void redis_client::Command(const char* cmd, const char* arg, redisCallbackFn *fn, void *privdata)
{
	if (!_context) {
		SetError("no connection");
		return;
	}
	void* priv = privdata;
	if (priv == nullptr) {
		priv = this;
	}
	if (flame::this_fiber() == NULL) {
		throw php::exception("need yield!", -1);
	} else {
		_fiber = flame::this_fiber();
	}
	redisAsyncCommand(_context, fn, priv, "%s %s", cmd, arg);
}

void redis_client::Command(int argc, const char **argv, const size_t *argvlen, redisCallbackFn *fn, void *privdata)
{
	if (!_context) {
		SetError("no connection");
		return;
	}
	void* priv = privdata;
	if (priv == nullptr) {
		priv = this;
	}
	if (flame::this_fiber() == NULL) {
		throw php::exception("need yield!", -1);
	} else {
		_fiber = flame::this_fiber();
	}
	redisAsyncCommandArgv(_context, fn, priv, argc, argv, argvlen);
}

}
}

