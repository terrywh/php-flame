#include "redis.h"

namespace flame {
namespace db {

php::value redis::convert_redis_reply(redisReply* reply) {
	php::value elem(nullptr);
	if (reply != nullptr) {
		switch(reply->type) {
		case REDIS_REPLY_INTEGER:
			elem = (std::int64_t)reply->integer;
		break;
		case REDIS_REPLY_STRING:
		case REDIS_REPLY_STATUS:
			elem = php::string(reply->str);
		break;
		case REDIS_REPLY_ERROR:
			elem   = php::value(nullptr);
			error_ = php::string(reply->str);
		break;
		case REDIS_REPLY_ARRAY: {
			php::array arr(reply->elements);
			for(int i = 0; i < reply->elements; ++i) {
				arr[i] = convert_redis_reply(reply->element[i]);
			}
			elem = arr;
		}
		break;
		case REDIS_REPLY_NIL:
			// elem = php::value(nullptr);
		break;
		}
	} else {
		error_ = php::string("empty reply");
	}
	return std::move(elem);
}

static void return_result(php::value& result, void* privdata) {
	if (privdata) {
		flame::fiber* fib = reinterpret_cast<redis::redisRequest*>(privdata)->fib;
		fib->next(std::move(result));
		delete reinterpret_cast<redis::redisRequest*>(privdata);
	}else{
		assert(0);
	}
}

void redis::null_callback(redisAsyncContext *c, void *r, void *privdata) {

}

void redis::default_callback(redisAsyncContext *c, void *r, void *privdata) {
	redis*   self = reinterpret_cast<redis*>(c->data);
	php::value rv = self->convert_redis_reply(reinterpret_cast<redisReply*>(r));
	return_result(rv, privdata);
}

void redis::return_pair_callback(redisAsyncContext *c, void *r, void *privdata) {
	redisReply* reply = reinterpret_cast<redisReply*>(r);
	redis*       self = reinterpret_cast<redis*>(privdata);
	php::value rv;
	if (reply == nullptr || reply->type != REDIS_REPLY_ARRAY) {
		assert(0);
	} else {
		php::array arr(reply->elements/2);
		for(int i = 0; i < reply->elements; i=i+2) { // i是key，i+1就是value
			const char* key = reply->element[i]->str;
			arr[key] = self->convert_redis_reply(reply->element[i+1]);
		}
		rv = std::move(arr);
	}
	return_result(rv, privdata);
}

void redis::hmget_callback(redisAsyncContext *c, void *r, void *privdata) {
	redisReply*   reply = reinterpret_cast<redisReply*>(r);
	redisRequest* cache = reinterpret_cast<redisRequest*>(privdata);
	redis*         self = reinterpret_cast<redis*>(c->data);
	php::array rv(reply->elements);
	if(reply == nullptr || reply->type != REDIS_REPLY_ARRAY) {
		assert(0);
	} else {
		for(int i = 0; i < reply->elements; ++i) { // key在Cache里取
			php::string& key = cache->cmd[i+2]; // 第一个是cmd，第二个是hash名，所以要加2
			rv[key.c_str()]  = self->convert_redis_reply(reply->element[i]);
		}
	}
	return_result(rv, privdata);
}

void redis::arg_key_callback(redisAsyncContext *c, void *r, void *privdata) {
	redisReply*   reply = reinterpret_cast<redisReply*>(r);
	redisRequest* cache = reinterpret_cast<redisRequest*>(privdata);
	redis*         self = reinterpret_cast<redis*>(c->data);
	php::array rv(reply->elements);
	if (reply == nullptr || reply->type != REDIS_REPLY_ARRAY) {
		assert(0);
	} else {
		for(int i = 0; i < reply->elements; ++i) { // key 在 Cache 里取
			php::string& key = cache->cmd[i+1]; // 第一个是cmd，所以要加1
			rv[key.c_str()]  = self->convert_redis_reply(reply->element[i]);
		}
	}
	return_result(rv, privdata);
}

void redis::quit_callback(redisAsyncContext *c, void *r, void *privdata) {
	redis*   self = reinterpret_cast<redis*>(c->data);
	php::value rv = self->convert_redis_reply(reinterpret_cast<redisReply*>(r));
	self->close();
	return_result(rv, privdata);
}

void redis::connect_callback(const redisAsyncContext *c, int status) {
	if (status != REDIS_OK) { // TODO 错误处理？
		std::printf("error: redis failed with '%s'\n", c->errstr);
		return;
	}
}

void redis::disconnect_callback(const redisAsyncContext *c, int status) {
	redis*    self = reinterpret_cast<redis*>(c->data);
	self->context_ = nullptr;
	if (status != REDIS_OK) { // TODO 错误处理？
		std::printf("error: redis failed with '%s'\n", c->errstr);
		return;
	}
}

redis::redis()
: context_(nullptr) {
}

redis::~redis() {
	close();
}

php::value redis::connect(php::array& arr) {
	close();
	php::string& host = arr["host"];
	int          port = arr["port"];
	context_ = redisAsyncConnect(host.c_str(), port);
	if (context_->err) {
		error_ = context_->err;
		return nullptr;
	}
	context_->data = this;
	redisLibuvAttach(context_, flame::loop);
	redisAsyncSetConnectCallback(context_, connect_callback);
	redisAsyncSetDisconnectCallback(context_, disconnect_callback);
	return nullptr;
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
	php::string&    cmd = params[0];
	redisCallbackFn* fn = redis::default_callback;
	php::array args;
	if (params.length() > 1) {
		args = params[1];
		if((cmd.c_str()[0] == 'z' || cmd.c_str()[0] == 'Z') && // Z* 函数 如果带有特殊标识需要特殊的回调
			strcasecmp(args[args.length()-1].to_string().c_str(), "WITHSCORES") == 0) {
			fn = redis::return_pair_callback;
		}
	}
	command(cmd.c_str(), args, fn);
	return flame::async();
}

php::value redis::connect(php::parameters& params) {
	php::array& arr = params[0];
	return connect(arr);
}

php::value redis::close(php::parameters& params) {
	close();
	return nullptr;
}

void redis::close() {
	if (context_) {
		redisAsyncDisconnect(context_);
		context_ = nullptr;
	}
}

php::value redis::hgetall(php::parameters& params) {
	command("HGETALL", params, return_pair_callback);
	return flame::async();
}

php::value redis::hmget(php::parameters& params) {
	command("HMGET", params, hmget_callback);
	return flame::async();
}

php::value redis::mget(php::parameters& params) {
	command("MGET", params, arg_key_callback);
	return flame::async();
}

void redis::subscribe_callback(redisAsyncContext *c, void *r, void *privdata) {
	redis*       self = reinterpret_cast<redis*>(c->data);
	php::value     rv = self->convert_redis_reply(reinterpret_cast<redisReply*>(r));
	php::callable& cb = reinterpret_cast<redisRequest*>(privdata)->cb;
	if (rv.is_array()) {
		php::array& arr = rv;
		cb(self, arr[1], arr[2]);
	} else {
		assert(0);
	}
}

php::value redis::subscribe(php::parameters& params) {
	php::array& ch = params[0];
	php::value  cb = params[1];
	command("SUBSCRIBE", ch, subscribe_callback, &cb);
	return flame::async();
}

php::value redis::quit(php::parameters& params) {
	php::array arr;
	command("QUIT", arr, quit_callback);
	return flame::async();
}

void redis::command(const char* cmd, php::parameters& params, redisCallbackFn* fn, php::value* cb) {
	redisRequest* data = new redisRequest;
	data->cmd.reserve(params.length() + 1);
	data->cmd[0] = php::string(cmd);
	if (cb != nullptr) data->cb = std::move(*cb);

	std::vector<const char*> argv;
	argv.push_back(data->cmd[0].c_str()); // 先放cmd
	for( int i=0; i < params.length(); ++i) { // 再放参数
		data->cmd[i+1] = params[i].to_string();
		argv.push_back(data->cmd[i+1].c_str());
	}
	command(argv.size(), (const char**)argv.data(), nullptr, fn, data);
}

void redis::command(const char* cmd, php::array& arr, redisCallbackFn *fn, php::value* cb) {
	redisRequest* data = new redisRequest;
	data->cmd.reserve(arr.length() + 1);
	data->cmd[0] = php::string(cmd);
	if (cb != nullptr) data->cb = std::move(*cb);

	std::vector<const char*> argv;
	argv.push_back(data->cmd[0].c_str()); // 先放cmd
	int i = 1;
	for( auto iter = arr.begin(); iter != arr.end(); ++iter) { // 再放参数
		data->cmd[i] = iter->second.to_string();
		argv.push_back(data->cmd[i].c_str());
		++i;
	}
	command(argv.size(), (const char**)argv.data(), nullptr, fn, data);
}

void redis::command(int argc, const char **argv, const size_t *argvlen, redisCallbackFn *fn, redisRequest *privdata) {
	privdata->fib = flame::this_fiber()->push(privdata);
	redisAsyncCommandArgv(context_, fn, privdata, argc, argv, argvlen);
}

}
}
