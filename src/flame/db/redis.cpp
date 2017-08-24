#include "redis.h"

namespace flame {
namespace db {


php::value redis::format_redis_result(redisReply* reply) {
	php::value elem(nullptr);
	if (reply != nullptr) {
		switch(reply->type) {
		case REDIS_REPLY_INTEGER:
			elem = (std::int64_t)reply->integer;
		break;
		case REDIS_REPLY_STRING:
			elem = php::string(reply->str);
		break;
		case REDIS_REPLY_ERROR:
			elem = php::value(nullptr);
			error_ = reply->str;
		break;
		case REDIS_REPLY_ARRAY: {
			php::array arr(reply->elements);
			for(int i = 0; i < reply->elements; ++i) {
				arr[i] = format_redis_result(reply->element[i]);
			}
			elem = arr;
		}
		break;
		case REDIS_REPLY_NIL:
			elem = php::value(nullptr);
		break;
		case REDIS_REPLY_STATUS:
			elem = reply->str;
		break;
		}
	} else {
		error_ = "empty reply";
	}
	return std::move(elem);
}

static void return_result(php::value& result, void* privdata) {
	if (privdata) {
		flame::fiber* fiber = reinterpret_cast<redis::redisRequest*>(privdata)->fiber;
		fiber->next(std::move(result));
		delete reinterpret_cast<redis::redisRequest*>(privdata);
	}
}

void redis::null_callback(redisAsyncContext *c, void *r, void *privdata) {

}

void redis::default_callback(redisAsyncContext *c, void *r, void *privdata) {
	redis* cli = reinterpret_cast<redis*>(c->data);
	php::value result = cli->format_redis_result(reinterpret_cast<redisReply*>(r));
	return_result(result, privdata);
}

void redis::return_pair_callback(redisAsyncContext *c, void *r, void *privdata) {
	redisReply* reply = reinterpret_cast<redisReply*>(r);
	redis*       self = reinterpret_cast<redis*>(privdata);
	php::value result;
	if (reply == nullptr || (reply->type == REDIS_REPLY_NIL)) {
		self->error_ = "empty reply";
	} else if (reply->type == REDIS_REPLY_ARRAY) {
		// i是key，i+1就是value
		php::array arr(reply->elements/2);
		for(int i = 0; i < reply->elements; i=i+2) {
			const char* key = reply->element[i]->str;
			arr[key] = self->format_redis_result(reply->element[i+1]);
		}
		result = arr;
	} else {
		self->error_ = "illegal reply";
	}
	return_result(result, privdata);
}

void redis::hmget_callback(redisAsyncContext *c, void *r, void *privdata) {
	redisReply*     reply = reinterpret_cast<redisReply*>(r);
	redisRequest*  cache = reinterpret_cast<redisRequest*>(privdata);
	redis*           self = reinterpret_cast<redis*>(c->data);
	php::array result(reply->elements);
	if (reply == nullptr || (reply->type == REDIS_REPLY_NIL)) {
		self->error_ = "empty reply";
	} else if (reply->type == REDIS_REPLY_ARRAY) {
		// key在Cache里取
		for(int i = 0; i < reply->elements; ++i) {
			// 第一个是cmd，第二个是hash名，所以要加2
			php::string& key = cache->cmd[i+2];
			result[key.c_str()] = self->format_redis_result(reply->element[i]);
		}
	} else {
		self->error_ = "illegal reply";
	}
	return_result(result, privdata);
}

void redis::arg_key_callback(redisAsyncContext *c, void *r, void *privdata) {
	redisReply*     reply = reinterpret_cast<redisReply*>(r);
	redisRequest*  cache = reinterpret_cast<redisRequest*>(privdata);
	redis*           self = reinterpret_cast<redis*>(c->data);
	php::array result(reply->elements);
	if (reply == nullptr || (reply->type == REDIS_REPLY_NIL)) {
		self->error_ = "empty reply";
	} else if (reply->type == REDIS_REPLY_ARRAY) {
		// key在Cache里取
		for(int i = 0; i < reply->elements; ++i) {
			// 第一个是cmd，所以要加1
			php::string& key = cache->cmd[i+1];
			result[key.c_str()] = self->format_redis_result(reply->element[i]);
		}
	} else {
		self->error_ = "illegal reply";
	}
	return_result(result, privdata);
}

void redis::quit_callback(redisAsyncContext *c, void *r, void *privdata) {
	redis*       self = reinterpret_cast<redis*>(c->data);
	php::value result = self->format_redis_result(reinterpret_cast<redisReply*>(r));
	self->close();
	return_result(result, privdata);
}

void redis::connect_callback(const redisAsyncContext *c, int status) {
	if (status != REDIS_OK) { // TODO 错误处理？
		std::printf("[redis error]: %s\n", c->errstr);
		return;
	}
}

void redis::disconnect_callback(const redisAsyncContext *c, int status) {
	redis* self = reinterpret_cast<redis*>(c->data);
	self->context_ = nullptr;
	if (status != REDIS_OK) { // TODO 错误处理？
		std::printf("[redis error] %s\n", c->errstr);
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
	if (context_) {
		// 已经连接过一次了？
		close();
	}
	php::string& host = arr["host"];
	int port = arr["port"];
	context_ = redisAsyncConnect(host.c_str(), port);
	if (context_->err) {
		error_ = context_->err;
		return nullptr;
	}
	context_->data = this;
	uv_loop_t* loop = uv_default_loop();
	redisLibuvAttach(context_,loop);
	redisAsyncSetConnectCallback(context_,connect_callback);
	redisAsyncSetDisconnectCallback(context_,disconnect_callback);
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
	php::string& cmd = params[0];
	php::array args;
	redisCallbackFn* fn = redis::default_callback;
	if (params.length() > 1) {
		args = params[1];
		if (strcasecmp(args[args.length()-1].to_string().c_str(),"WITHSCORES") == 0) {
			// 如果带withscores，就换一下default函数
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
	redis* self = reinterpret_cast<redis*>(c->data);
	php::value result = self->format_redis_result(reinterpret_cast<redisReply*>(r));
	php::callable& cb = reinterpret_cast<redisRequest*>(privdata)->cb;
	if (result.is_null()) {
		// TODO 错误处理？
		// php::exception("subscribe error",-1);
	} else {
		php::array& arr = result;
		cb(self, arr[1], arr[2]);
	}
}

php::value redis::subscribe(php::parameters& params) {
	php::array& channel = params[0];
	php::value cb = params[1];
	command("SUBSCRIBE", channel, subscribe_callback, &cb);
	return flame::async();
}

php::value redis::quit(php::parameters& params) {
	php::array arr;
	command("QUIT", arr, quit_callback);
	return flame::async();
}

void redis::command(const char* cmd, php::parameters& params, redisCallbackFn* fn, php::value* cb) {
	redisRequest* data = new redisRequest;
	data->cmd[(int)0] = php::string(cmd);
	if (cb != nullptr) data->cb = std::move(*cb);

	std::vector<const char*> argv;
	// 先放cmd
	argv.push_back(cmd);
	// 再放参数
	for( int i=0; i < params.length(); ++i) {
		if (params[i].is_string()) {
			data->cmd[i+1] = params[i];
		} else {
			data->cmd[i+1] = params[i].to_string();
		}
		argv.push_back(((php::string*)&data->cmd[i+1])->data());
	}
	command(argv.size(), (const char**)argv.data(), nullptr, fn, data);
}

void redis::command(const char* cmd, php::array& arr, redisCallbackFn *fn, php::value* cb) {
	redisRequest* data =new redisRequest;
	data->cmd[(int)0] = php::string(cmd);
	if (cb != nullptr) data->cb = std::move(*cb);

	std::vector<const char*> argv;
	// 先放cmd
	argv.push_back(cmd);
	// 再放参数
	int argv_index = 1;
	for( auto iter = arr.begin(); iter != arr.end(); ++iter) {
		if (iter->second.is_string()) {
			data->cmd[argv_index] = iter->second;
		} else {
			data->cmd[argv_index] = iter->second.to_string();
		}
		argv.push_back(((php::string*)&data->cmd[argv_index])->data());
		++argv_index;
	}
	command(argv.size(), (const char**)argv.data(), nullptr, fn, data);
}

void redis::command(int argc, const char **argv, const size_t *argvlen, redisCallbackFn *fn, redisRequest *privdata) {
	privdata->fiber = flame::this_fiber()->push(privdata);
	redisAsyncCommandArgv(context_, fn, privdata, argc, argv, argvlen);
}

}
}
