#include "../coroutine.h"
#include "redis.h"

namespace flame {
namespace db {

static php::value convert_redis_reply(redisReply* reply) {
	php::value rv(nullptr);
	if (reply != nullptr) {
		switch(reply->type) {
		case REDIS_REPLY_INTEGER:
			rv = (std::int64_t)reply->integer;
		break;
		case REDIS_REPLY_STRING:
		case REDIS_REPLY_STATUS:
			rv = php::string(reply->str);
		break;
		case REDIS_REPLY_ERROR:
			rv = php::make_exception(reply->str);
		break;
		case REDIS_REPLY_ARRAY: {
			php::array arr(reply->elements);
			for(int i = 0; i < reply->elements; ++i) {
				arr[i] = convert_redis_reply(reply->element[i]);
			}
			rv = arr;
		}
		break;
		case REDIS_REPLY_NIL:
		break;
		}
	} else {
		rv = php::make_exception("EMPTY empty reply", -2);
	}
	return std::move(rv);
}

void redis::cb_dummy(redisAsyncContext *c, void *r, void *privdata) {

}

void redis::cb_default(redisAsyncContext *c, void *r, void *privdata) {
	redis*       self = reinterpret_cast<redis*>(c->data);
	redisReply* reply = reinterpret_cast<redisReply*>(r);
	redisRequest* req = reinterpret_cast<redisRequest*>(privdata);
	php::value     rv = convert_redis_reply(reply);
	req->co->next(rv);
	delete req;
}

void redis::cb_assoc_2(redisAsyncContext *c, void *r, void *privdata) {
	redis*       self = reinterpret_cast<redis*>(c->data);
	redisReply* reply = reinterpret_cast<redisReply*>(r);
	redisRequest* req = reinterpret_cast<redisRequest*>(privdata);

	if (reply == nullptr || reply->type != REDIS_REPLY_ARRAY) {
		req->co->fail(php::make_exception("ILLEGAL illegal reply", -2));
	} else {
		php::array rv(reply->elements/2);
		for(int i = 0; i < reply->elements; i=i+2) { // i 是 key，i+1 就是value
			const char* key = reply->element[i]->str;
			rv[key] = convert_redis_reply(reply->element[i+1]);
		}
		req->co->next(std::move(rv));
	}
	delete req;
}

void redis::cb_assoc_1(redisAsyncContext *c, void *r, void *privdata) {
	redis*       self = reinterpret_cast<redis*>(c->data);
	redisReply* reply = reinterpret_cast<redisReply*>(r);
	redisRequest* req = reinterpret_cast<redisRequest*>(privdata);


	if (reply == nullptr || reply->type != REDIS_REPLY_ARRAY) {
		req->co->fail(php::make_exception("ILLEGAL illegal reply", -2));
	} else {
		php::array rv(reply->elements);
		for(int i = 0; i < reply->elements; ++i) {
			php::string& key = req->key[i];
			rv[key.c_str()]  = convert_redis_reply(reply->element[i]);
		}
		req->co->next(std::move(rv));
	}
	delete req;
}

void redis::cb_subscribe(redisAsyncContext *c, void *r, void *privdata) {
	redis*       self = reinterpret_cast<redis*>(c->data);
	redisReply* reply = reinterpret_cast<redisReply*>(r);
	redisRequest* req = reinterpret_cast<redisRequest*>(privdata);
	php::value     rv = convert_redis_reply(reply);

	if (rv.is_array()) {
		php::array& arr = rv;
		req->cb(arr[1], arr[2]);
	} else {
		req->co->fail(php::make_exception("ILLEGAL illegal reply", -2));
		delete req;
	}
}

void redis::cb_quit(redisAsyncContext *c, void *r, void *privdata) {
	redis*       self = reinterpret_cast<redis*>(c->data);
	redisReply* reply = reinterpret_cast<redisReply*>(r);
	redisRequest* req = reinterpret_cast<redisRequest*>(privdata);
	php::value     rv = convert_redis_reply(reply);
	self->close();
	req->co->next(rv);
}

void redis::cb_connect(const redisAsyncContext *c, int status) {
	if (status != REDIS_OK) { // TODO 错误处理？
		std::printf("error: redis failed with '%s'\n", c->errstr);
		return;
	}
	redis* self = static_cast<redis*>(c->data);
	// 连接建立的过程
	if(self->connect_ != nullptr) {
		self->connect_->next();
		self->connect_ = nullptr;
	}
}

void redis::cb_disconnect(const redisAsyncContext *c, int status) {
	redis* self = reinterpret_cast<redis*>(c->data);
	if (status != REDIS_OK) { // TODO 错误处理？
		std::printf("error: redis failed with '%s'\n", c->errstr);
		return;
	}
	if(self->context_ != nullptr) {
		// 自动重连
		uv_timer_stop(&self->connect_interval);
		uv_timer_start(&self->connect_interval, cb_connect_interval, std::rand() % 3000 + 2000, 0);
	}
	self->context_ = nullptr;
}

redis::redis()
: context_(nullptr) {
	uv_timer_init(flame::loop, &connect_interval);
	connect_interval.data = this;
}

redis::~redis() {
	close();
}

void redis::cb_connect_interval(uv_timer_t* tm) {
	redis* self = reinterpret_cast<redis*>(tm->data);
	if(self->context_ != nullptr) self->connect();
}

void redis::connect() {
	context_ = redisAsyncConnect(host_.c_str(), port_);
	if (context_->err) {
		std::printf("error: redis failed with '%s'\n", context_->errstr);
		// 自动重连
		uv_timer_stop(&connect_interval);
		uv_timer_start(&connect_interval, cb_connect_interval, std::rand() % 3000 + 2000, 0);
		return;
	}
	context_->data = this;
	redisLibuvAttach(context_, flame::loop);
	redisAsyncSetConnectCallback(context_, cb_connect);
	redisAsyncSetDisconnectCallback(context_, cb_disconnect);
}
void redis::cb_connect_timeout(uv_timer_t* tm) {
	redis* self = static_cast<redis*>(tm->data);
	if(self->connect_ != nullptr) {
		redisAsyncDisconnect(self->context_);
		self->context_ = nullptr;

		self->connect_->fail("redis connect timeout");
		self->connect_ = nullptr;
	}
}
php::value redis::connect(php::parameters& params) {
	host_ = params[0];
	port_ = params[1];
	int timeout = 2500;
	if(params.length() > 2) {
		timeout = params[2];
	}
	connect();
	connect_ = flame::coroutine::current;
	uv_timer_stop(&connect_interval);

	uv_timer_start(&connect_interval, cb_connect_timeout, timeout, 0);
	return flame::async();
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

php::value redis::__call(php::parameters& params) {
	php::array& argv = params[1];
	std::vector<zval>  args;
	for(auto i=argv.begin(); i!=argv.end();++i) {
		args.push_back(*static_cast<zval*>(i->second));
	}
	php::parameters argx(args.size(), args.data());
	exec(params[0], argx, 0, params.length(), cb_default);
	return flame::async();
}

php::value redis::hgetall(php::parameters& params) {
	exec(php::string("HGETALL", 7), params, 0, params.length(), cb_assoc_2);
	return flame::async();
}

php::value redis::hmget(php::parameters& params) {
	exec(php::string("HMGET", 5), params, 0, 1, cb_assoc_1);
	return flame::async();
}

php::value redis::mget(php::parameters& params) {
	exec(php::string("MGET", 4), params, 0, 0, cb_assoc_1);
	return flame::async();
}

php::value redis::subscribe(php::parameters& params) {
	if(!params[0].is_callable()) {
		throw php::exception("callback is required");
	}
	exec(php::string("SUBSCRIBE",9), params, 1, params.length(), cb_subscribe, params[0]);
	return flame::async();
}

php::value redis::quit(php::parameters& params) {
	exec(php::string("QUIT",4), params, params.length(), params.length(), cb_quit);
	return flame::async();
}

void redis::exec(const php::string& cmd, php::parameters& params, int start, int offset, redisCallbackFn* fn, const php::value& cb) {
	// TODO 命令超时？
	redisRequest* req = new redisRequest;
	req->cb  = cb;
	req->co  = flame::coroutine::current;
	req->ref = this; // 当前对象的引用
	std::vector<const char*> argv;
	std::vector<size_t>      argl;
	argv.push_back(cmd.c_str());
	argl.push_back(cmd.length());
	for(int i=start; i<params.length(); ++i) { // 再放参数
		php::string& str = params[i].to_string();
		if(i >= offset) {
			req->key.push_back(str);
		}
		argv.push_back(str.c_str());
		argl.push_back(str.length());
	}
	int error = redisAsyncCommandArgv(context_, fn, req, argv.size(), argv.data(), argl.data());
	if(error != 0) {
		delete req;
		flame::coroutine::current->fail("UNKONWN failed to send command (disconnected ? subscribed ?)");
	}
}

}
}
