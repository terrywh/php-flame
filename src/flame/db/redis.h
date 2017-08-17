#include "../fiber.h"
//#define MY_DEBUG

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);

namespace flame {
namespace db {

class redis: public php::class_base {
public:
	typedef struct { // 由于特殊使用方式，这里借鉴 hiredis 命名
		php::array    cmd;
		flame::fiber* fiber;
		php::callable cb;
	} redisRequest;

	redis();
	virtual ~redis();
	php::value __construct(php::parameters& params);
	php::value __call(php::parameters& params);
	php::value connect(php::parameters& params);
	php::value close(php::parameters& params);
	php::value getlasterror(php::parameters& params)
	{
		if (error_.is_null())
			return php::string();
		else
			return error_;
	};
	php::value hgetall(php::parameters& params);
	php::value hmget(php::parameters& params);
	php::value mget(php::parameters& params);
	php::value quit(php::parameters& params);
	php::value subscribe(php::parameters& params);

	php::value error_;
private:
	php::value format_redis_result(redisReply* reply);
	static void connect_callback(const redisAsyncContext *c, int status);
	static void disconnect_callback(const redisAsyncContext *c, int status);
	static void null_callback(redisAsyncContext *c, void *r, void *privdata);
	// 默认回调，按照redis返回的type格式化返回
	static void default_callback(redisAsyncContext *c, void *r, void *privdata);
	// 当返回的array要按顺序成对格式化成key=>value格式的时候使用此回调
	static void return_pair_callback(redisAsyncContext *c, void *r, void *privdata);
	// 当需要把请求参数保存下来作为array的key，返回的array当成value的时候使用此回调
	static void arg_key_callback(redisAsyncContext *c, void *r, void *privdata);
	// hmget专用，因为第二个参数是hash，有点特殊
	static void hmget_callback(redisAsyncContext *c, void *r, void *privdata);
	static void quit_callback(redisAsyncContext *c, void *r, void *privdata);
	static void subscribe_callback(redisAsyncContext *c, void *r, void *privdata);
	void command(const char* cmd, php::parameters& params, redisCallbackFn* fn, php::value* cb = nullptr);
	void command(const char* cmd, php::array& arr, redisCallbackFn *fn, php::value* cb = nullptr);
	void command(int argc, const char **argv, const size_t *argvlen, redisCallbackFn *fn, redisRequest *privdata);
	php::value connect(php::array& arr);
	void close();
	redisAsyncContext *context_;
};

}
}
