#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "redis.h"

namespace flame {
namespace db {

	static php::value convert_redis_reply(redisReply* reply) {
		php::value rv(nullptr);

		switch(reply->type) {
		case REDIS_REPLY_INTEGER:
			rv = (std::int64_t)reply->integer;
		break;
		case REDIS_REPLY_STRING:
		case REDIS_REPLY_STATUS:
			rv = php::string(reply->str);
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
		case REDIS_REPLY_ERROR:
		default:
			assert(0);
		}
		return std::move(rv);
	}
	redis::redis()
	: context_(nullptr)
	, connect_(nullptr)
	, current_(nullptr) {
		connect_interval = (uv_timer_t*)malloc(sizeof(uv_timer_t));
		uv_timer_init(flame::loop, connect_interval);
		connect_interval->data = this;
	}
	redis::~redis() {
		close();
	}
	void redis::cb_connect_interval(uv_timer_t* tm) {
		redis* self = reinterpret_cast<redis*>(tm->data);
		if(self->context_ != nullptr) self->connect();
	}
	void redis::cb_disconnect(const redisAsyncContext *c, int status) {
		redis* self = reinterpret_cast<redis*>(c->data);
		if (status != REDIS_OK) { // TODO 错误处理？
			std::fprintf(stderr, "error: redis failed with '%s'\n", c->errstr);
			return;
		}
		if(self->context_ != nullptr) {
			// 自动重连
			uv_timer_stop(self->connect_interval);
			uv_timer_start(self->connect_interval, cb_connect_interval, std::rand() % 3000 + 2000, 0);
		}
	}
	void redis::cb_connect(const redisAsyncContext *c, int status) {
		if (status != REDIS_OK) { // TODO 错误处理？
			std::fprintf(stderr, "error: redis failed with '%s'\n", c->errstr);
			return;
		}
		redis* self = static_cast<redis*>(c->data);
		// 连接建立的过程
		if(self->connect_ != nullptr) {
			self->connect_->co->next();
			delete self->connect_;
			self->connect_ = nullptr;
		}
	}
	void redis::connect() {
		context_ = redisAsyncConnect(host_.c_str(), port_);
		if (context_->err) {
			std::fprintf(stderr, "error: redis failed with '%s'\n", context_->errstr);
			// 自动重连
			uv_timer_stop(connect_interval);
			uv_timer_start(connect_interval, cb_connect_interval, std::rand() % 3000 + 2000, 0);
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

			self->connect_->co->fail("redis connect timeout", 0);
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
		connect_ = new redis_request_t {
			coroutine::current, nullptr
		};
		uv_timer_stop(connect_interval);
		uv_timer_start(connect_interval, cb_connect_timeout, timeout, 0);

		return flame::async(this);
	}

	php::value redis::close(php::parameters& params) {
		close();
		return nullptr;
	}

	void redis::close() {
		if (context_) {
			redisAsyncContext* ctx = context_;
			context_ = nullptr;
			redisAsyncDisconnect(ctx);
		}
		if(connect_interval) {
			uv_timer_stop(connect_interval);
			uv_close((uv_handle_t*)connect_interval, free_handle_cb);
		}
	}
	void redis::cb_default(redisAsyncContext *c, void *r, void *privdata) {
		redis*       self = reinterpret_cast<redis*>(c->data);
		redisReply* reply = reinterpret_cast<redisReply*>(r);
		redis_request_t* ctx = reinterpret_cast<redis_request_t*>(privdata);
		if(reply == nullptr) {
			ctx->co->next();
		}else if(reply->type == REDIS_REPLY_ERROR) {
			ctx->co->fail(reply->str);
		}else{
			ctx->co->next(convert_redis_reply(reply));
		}
		delete ctx;
	}
	void redis::cb_assoc_even(redisAsyncContext *c, void *r, void *privdata) {
		redis*       self = reinterpret_cast<redis*>(c->data);
		redisReply* reply = reinterpret_cast<redisReply*>(r);
		redis_request_t* ctx = reinterpret_cast<redis_request_t*>(privdata);

		if (reply == nullptr) {
			ctx->co->next(nullptr);
		}else if(reply->type == REDIS_REPLY_ERROR) {
			ctx->co->fail(reply->str);
		}else if(reply->type != REDIS_REPLY_ARRAY) {
			ctx->co->fail("ILLEGAL illegal reply", -2);
		} else {
			php::array rv(reply->elements/2);
			for(int i = 0; i < reply->elements; i=i+2) { // i 是 key，i+1 就是value
				redisReply* key = reply->element[i];
				rv.at(key->str, key->len) = convert_redis_reply(reply->element[i+1]);
			}
			ctx->co->next(std::move(rv));
		}
		delete ctx;
	}
	php::value redis::__call(php::parameters& params) {
		php::string& name = params[0];
		php::strtoupper_inplace(name.data(), name.length());
		php::array&  data = params[1];
		std::vector<const char*> argv;
		std::vector<size_t>      argl;
		redisCallbackFn*           cb;
		if(std::strncmp(name.c_str(), "MGET", 4) == 0) {
			cb = cb_assoc_keys;
		}else if(std::strncmp(name.c_str(), "HGETALL", 7) == 0) {
			cb = cb_assoc_even;
		}else{
			cb = cb_default;
		}
		argv.push_back(name.c_str());
		argl.push_back(name.length());
		redis_request_t* ctx = new redis_request_t {
			coroutine::current, nullptr
		};
		for(int i=0; i<data.length(); ++i) {
			php::string str = data[i].to_string();
			if(name.c_str()[0] == 'Z' && strncasecmp(str.c_str(), "WITHSCORES", 10) == 0) {
				cb = cb_assoc_even;
			}else{
				ctx->key.push_back(str);
			}
			argv.push_back(str.c_str());
			argl.push_back(str.length());
		}
		exec(argv.data(), argl.data(), argv.size(), ctx, cb);
		return flame::async(this);
	}
	void redis::cb_assoc_keys(redisAsyncContext *c, void *r, void *privdata) {
		redis*       self = reinterpret_cast<redis*>(c->data);
		redisReply* reply = reinterpret_cast<redisReply*>(r);
		redis_request_t* ctx = reinterpret_cast<redis_request_t*>(privdata);

		if (reply == nullptr) {
			ctx->co->next(nullptr);
		}else if(reply->type == REDIS_REPLY_ERROR) {
			ctx->co->fail(reply->str);
		}else if(reply->type != REDIS_REPLY_ARRAY) {
			ctx->co->fail("ILLEGAL illegal reply", -2);
		} else {
			php::array rv(reply->elements);
			for(int i = 0; i < reply->elements; ++i) {
				rv[ctx->key[i]] = convert_redis_reply(reply->element[i]);
			}
			ctx->co->next(std::move(rv));
		}
		delete ctx;
	}
	php::value redis::hmget(php::parameters& params) {
		php::string& name = params[0];
		std::vector<const char*> argv;
		std::vector<size_t>      argl;
		argv.push_back("HMGET");
		argl.push_back(5);
		argv.push_back(name.c_str());
		argl.push_back(name.length());
		redis_request_t* ctx = new redis_request_t {
			coroutine::current, nullptr
		};
		for(int i=1; i<params.length();++i) {
			php::string str = params[i].to_string();
			argv.push_back(str.c_str());
			argl.push_back(str.length());
			ctx->key.push_back(str);
		}
		exec(argv.data(), argl.data(), argv.size(), ctx, cb_assoc_keys);
		return flame::async(this);
	}
	void redis::cb_subscribe(redisAsyncContext *c, void *r, void *privdata) {
		redis*          self = reinterpret_cast<redis*>(c->data);
		redisReply*    reply = reinterpret_cast<redisReply*>(r);
		redis_request_t* ctx = reinterpret_cast<redis_request_t*>(privdata);
		
		if (reply == nullptr) {
			ctx->co->fail("ILLEGAL illegal reply", -2);
			self->current_ = nullptr;
			delete ctx;
		}else if(reply->type == REDIS_REPLY_ERROR) {
			ctx->co->fail(reply->str);
			self->current_ = nullptr;
			delete ctx;
		}else if(reply->type != REDIS_REPLY_ARRAY) {
			ctx->co->fail("ILLEGAL illegal reply", -2);
			self->current_ = nullptr;
			delete ctx;
		}else{
			php::array rv = convert_redis_reply(reply);
			php::string& type = rv[0];
			if(std::strncmp(type.c_str(), "subscribe", 9) == 0) {
				//
			}else if(std::strncmp(type.c_str(), "message", 7) == 0) {
				coroutine::start(ctx->cb, rv[1], rv[2]);
			}else if(self->current_ != nullptr && std::strncmp(type.c_str(), "unsubscribe", 7) == 0) {
				self->current_ = nullptr;
				ctx->co->next();
				delete ctx;
			}
		}
	}
	php::value redis::subscribe(php::parameters& params) {
		if(current_ != nullptr) {
			throw php::exception("failed to subscribe: already in progress");
		}
		if(!params[params.length()-1].is_callable()) {
			throw php::exception("failed to subscribe: callback is required");
		}
		std::vector<const char*> argv;
		std::vector<size_t>      argl;
		argv.push_back("SUBSCRIBE");
		argl.push_back(9);
		redis_request_t* ctx = new redis_request_t {
			coroutine::current, nullptr, params[params.length()-1]
		};
		ctx->key.push_back(php::string("UNSUBSCRIBE", 11));
		for(int i=0; i<params.length()-1;++i) {
			php::string str = params[i].to_string();
			argv.push_back(str.c_str());
			argl.push_back(str.length());
		}
		exec(argv.data(), argl.data(), argv.size(), ctx, cb_subscribe);
		current_ = ctx;
		return flame::async(this);
	}
	php::value redis::psubscribe(php::parameters& params) {
		if(current_ != nullptr) {
			throw php::exception("failed to psubscribe: already in progress");
		}
		if(!params[params.length()-1].is_callable()) {
			throw php::exception("failed to psubscribe: callback is required");
		}
		std::vector<const char*> argv;
		std::vector<size_t>      argl;
		argv.push_back("PSUBSCRIBE");
		argl.push_back(10);
		redis_request_t* ctx = new redis_request_t {
			coroutine::current, nullptr, params[params.length()-1]
		};
		ctx->key.push_back(php::string("UNPSUBSCRIBE", 12));
		for(int i=0; i<params.length()-1;++i) {
			php::string str = params[i].to_string();
			argv.push_back(str.c_str());
			argl.push_back(str.length());
		}
		exec(argv.data(), argl.data(), argv.size(), ctx, cb_subscribe);
		current_ = ctx;
		return flame::async(this);
	}
	php::value redis::stop_all(php::parameters& params) {
		if(current_ == nullptr) return nullptr;
		if(params.length() > 0) {
			throw php::exception("failed to punsubscribe: cannot have parameters");
		}
		std::vector<const char*> argv;
		std::vector<size_t>      argl;

		argv.push_back(current_->key.front().c_str());
		argl.push_back(current_->key.front().length());
		exec(argv.data(), argl.data(), argv.size(), nullptr, nullptr);
		return nullptr;
	}
	void redis::cb_quit(redisAsyncContext *c, void *r, void *privdata) {
		redis*       self = reinterpret_cast<redis*>(c->data);
		redisReply* reply = reinterpret_cast<redisReply*>(r);
		redis_request_t* ctx = reinterpret_cast<redis_request_t*>(privdata);
		php::value rv = convert_redis_reply(reply);
		self->close();
		ctx->co->next(rv);
		delete ctx;
	}
	php::value redis::quit(php::parameters& params) {
		std::vector<const char*> argv;
		std::vector<size_t>      argl;
		argv.push_back("QUIT");
		argl.push_back(4);
		redis_request_t* ctx = new redis_request_t {
			coroutine::current, nullptr
		};
		exec(argv.data(), argl.data(), argv.size(), ctx, cb_quit);
		return flame::async(this);
	}
	void redis::exec(const char** argv, const size_t* argl, size_t count, redis_request_t* req, redisCallbackFn* fn) {
		// TODO 命令超时？
		int error = redisAsyncCommandArgv(context_, fn, req, count, argv, argl);
		if(error != 0) {
			delete req;
			throw php::exception("UNKONWN failed to send command (disconnected ? subscribed ?)", 0);
		}
	}

}
}
