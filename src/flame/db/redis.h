#pragma once

namespace flame {
class coroutine;

namespace db {

class redis: public php::class_base {
public:
	typedef struct request_t {
		flame::coroutine*         co;
		php::value               ref;
		php::callable             cb;
		std::vector<php::string> key;
	} redis_request_t; // 由于特殊使用方式，这里借鉴 hiredis 命名
	redis();
	~redis();
	php::value connect(php::parameters& params);
	php::value close(php::parameters& params);

	php::value __call(php::parameters& params);
	void __call(php::string& name, php::array& data);
	php::value hmget(php::parameters& params);
	php::value subscribe(php::parameters& params);
	php::value psubscribe(php::parameters& params);
	php::value stop_all(php::parameters& params);
	php::value quit(php::parameters& params);
private:
	redisAsyncContext* context_;
	coroutine*         connect_; // 连接回调
	redis_request_t*   current_; // 用于记录 subscribe / psubscribe 状态
	std::shared_ptr<php_url> url_;
	// 生命周期需与 PHP 对象不符，故使用动态分配
	uv_timer_t* connect_interval;
	void connect();
	static void cb_connect_interval(uv_timer_t* tm);
	void close();
	void exec(const char** argv, const size_t* lens, size_t count, redis_request_t* req, redisCallbackFn* fn);
	static void cb_connect_timeout(uv_timer_t* tm);
	static void cb_connect(const redisAsyncContext *c, int status);
	static void cb_connect_auth(php::value& rv, void* data);
	static void cb_connect_select(php::value& rv, void* data);
	static void cb_disconnect(const redisAsyncContext *c, int status);
	// 默认回调，按照redis返回的type格式化返回
	static void cb_default(redisAsyncContext *c, void *r, void *privdata);
	// 当返回的array要按顺序成对格式化成key=>value格式的时候使用此回调
	static void cb_assoc_keys(redisAsyncContext *c, void *r, void *privdata);
	// 当需要把请求参数保存下来作为array的key，返回的array当成value的时候使用此回调
	static void cb_assoc_even(redisAsyncContext *c, void *r, void *privdata);
	// hmget专用，因为第二个参数是hash，有点特殊
	// static void cb_hmget(redisAsyncContext *c, void *r, void *privdata);
	static void cb_quit(redisAsyncContext *c, void *r, void *privdata);
	static void cb_subscribe(redisAsyncContext *c, void *r, void *privdata);
	static void cb_stop(redisAsyncContext *c, void *r, void *privdata);

};

}
}
