#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "../log/log.h"
#include "redis.h"

namespace flame {
namespace db {

	static php::value convert_redis_reply(redisReply* reply) {
		php::value rv;

		switch(reply->type) {
		case REDIS_REPLY_INTEGER:
			rv = (std::int64_t)reply->integer;
		break;
		case REDIS_REPLY_STRING:
		case REDIS_REPLY_STATUS:
			rv = php::string(reply->str);
		break;
		case REDIS_REPLY_ARRAY: {
			php::array arr(reply->elements + 4);
			for(int i = 0; i < reply->elements; ++i) {
				arr[i] = convert_redis_reply(reply->element[i]);
			}
			rv = std::move(arr);
		}
		break;
		case REDIS_REPLY_NIL:
			rv = nullptr;
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
			log::default_logger->write(
				fmt::format("(FAIL) redis connection dropped: ({0}) {1}", status, c->errstr)
			);
		}
		// 非主动关闭流程，需要尝试重连
		if(self->context_ != nullptr) {
			// 自动重连
			uv_timer_stop(self->connect_interval);
			uv_timer_start(self->connect_interval, cb_connect_interval, std::rand() % 3000 + 2000, 0);
		}
	}
	void redis::cb_connect(const redisAsyncContext *c, int status) {
		redis* self = static_cast<redis*>(c->data);
		// 连接建立的过程
		coroutine* co = self->connect_;
		self->connect_ = nullptr;
		if(co != nullptr && status == REDIS_OK) {
			co->next();
		}else if(co != nullptr && status != REDIS_OK) {
			co->fail(c->errstr, status);
		}else if(status == REDIS_OK){
			
		}else{
			log::default_logger->write(
				fmt::format("(FAIL) cannot establish redis connection: ({0}) {1}", status, c->errstr)
			);
		}
	}
	void redis::cb_connect_auth(php::value& rv, void* data) {
		redis* self = reinterpret_cast<redis*>(data);
		
		if(self->url_->user == nullptr || self->url_->pass == nullptr) {
			coroutine::current->next();
			return;
		}
		php::string method("auth",4);
		php::array  params(4);
		params[0] = php::string(self->url_->pass);
		self->__call(method, params, cb_default);
	}
	void redis::cb_connect_select(php::value& rv, void* data) {
		redis* self = reinterpret_cast<redis*>(data);
		
		if(self->url_->path == nullptr || strlen(self->url_->path) <= 1) {
			coroutine::current->next();
			return;
		}
		php::string method("select",6);
		php::array  params(4);
		params[0] = php::string(self->url_->path + 1); // 去除首部 / 斜线
		self->__call(method, params, cb_default);
	}
	void redis::connect() {
		context_ = redisAsyncConnect(url_->host, url_->port);
		if (context_->err) {
			log::default_logger->write(
				fmt::format("(FAIL) cannot establish redis connection: ({0}) {1}", context_->err, context_->errstr)
			);
			// 自动重连
			uv_timer_stop(connect_interval);
			uv_timer_start(connect_interval, cb_connect_interval, std::rand() % 3000 + 2000, 0);
			return;
		}
		context_->data = this;
		redisLibuvAttach(context_, flame::loop);
		connect_ = coroutine::current;
		connect_->async(cb_connect_auth, this);
		connect_->async(cb_connect_select, this);
		redisAsyncSetConnectCallback(context_, cb_connect);
		redisAsyncSetDisconnectCallback(context_, cb_disconnect);
	}
	void redis::cb_connect_timeout(uv_timer_t* tm) {
		redis* self = static_cast<redis*>(tm->data);
		if(self->connect_ != nullptr) {
			redisAsyncDisconnect(self->context_);
			self->context_ = nullptr;

			self->connect_->fail("redis connect timeout", ETIMEDOUT);
			self->connect_ = nullptr;
		}
	}
	php::value redis::connect(php::parameters& params) {
		if(params.length() == 2) {
			char cache[256];
			php::string host = params[0];
			int port = params[1];
			int size = sprintf(cache, "redis://%s:%d/", host.c_str(), port);
			url_ = php::parse_url(cache, size);
		}else{
			php::string url = params[0];
			url_ = php::parse_url(url.c_str(), url.length());
		}
		if(!url_ || strncasecmp("redis", url_->scheme, 5) != 0) {
			throw php::exception("illegal connection uri");
		}
		if(!url_->port) url_->port = 6379;
		
		int timeout = 2500;
		if(params.length() > 2) {
			timeout = params[2];
		}
		connect();
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
			connect_interval = nullptr;
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
			php::array rv(reply->elements/2 + 4);
			for(int i = 1; i < reply->elements; i=i+2) { // i+1 是 key，i 是val
				redisReply* key = reply->element[i-1];
				rv.at(key->str, key->len) = convert_redis_reply(reply->element[i]);
			}
			ctx->co->next(std::move(rv));
		}
		delete ctx;
	}
	php::value redis::__call(php::parameters& params) {
		php::string& name = static_cast<php::string&>(params[0]);
		php::array&  data = static_cast<php::array&>(params[1]);
		redisCallbackFn* cb;
		php::strtoupper_inplace(name.data(), name.length());
		if(std::strncmp(name.c_str(), "MGET", 4) == 0) {
			cb = cb_assoc_keys;
		}else if(std::strncmp(name.c_str(), "HGETALL", 7) == 0) {
			cb = cb_assoc_even;
		}else if(name.c_str()[0] == 'Z' && data.length() > 1) {
			php::string str = data[data.length()-1].to_string();
			if(strncasecmp(str.c_str(), "WITHSCORES", 10) == 0) {
				cb = cb_assoc_even;
			}else{
				cb = cb_default;
			}
		}else{
			cb = cb_default;
		}
		__call(name, data, cb);
		return flame::async(this);
	}
	void redis::__call(php::string& name, php::array& data, redisCallbackFn* cb) {
		std::vector<const char*> argv;
		std::vector<size_t>      argl;
	
		argv.push_back(name.c_str());
		argl.push_back(name.length());
		redis_request_t* ctx = new redis_request_t {
			coroutine::current, nullptr
		};
		for(int i=0; i<data.length(); ++i) {
			php::string str = data[i].to_string();
			ctx->key.push_back(str);
			argv.push_back(str.c_str());
			argl.push_back(str.length());
		}
		exec(argv.data(), argl.data(), argv.size(), ctx, cb);
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
			php::array rv(reply->elements + 4);
			for(int i = 0; i < reply->elements; ++i) {
				rv[ctx->key[i]] = convert_redis_reply(reply->element[i]);
			}
			ctx->co->next(std::move(rv));
		}
		delete ctx;
	}
	php::value redis::hmget(php::parameters& params) {
		php::string& name = static_cast<php::string&>(params[0]);
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
			php::string& str = params[i].to_string();
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
			php::array    rv = convert_redis_reply(reply);
			php::string type = rv[0];
			if(std::strncmp(type.c_str(), "subscribe", 9) == 0) {
				//
			}else if(std::strncmp(type.c_str(), "message", 7) == 0) {
				coroutine::create(ctx->cb, {rv[1], rv[2]})->start();
			}else if(self->current_ != nullptr && std::strncmp(type.c_str(), "unsubscribe", 11) == 0) {
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
			php::string& str = params[i].to_string();
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
			php::string& str = params[i].to_string();
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
