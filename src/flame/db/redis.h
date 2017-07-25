#include "../fiber.h"
//#define MY_DEBUG

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);

namespace flame {
namespace db {

struct redis_cmd_cache {
	std::vector<std::string> data;
};

class redis: public php::class_base {
public:
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
	php::value quit(php::parameters& params);
	php::value subscribe(php::parameters& params);
	
	void set_context(redisAsyncContext *context) { context_ = context; };

	flame::fiber* _fiber;
private:
	// 默认回调，按照redis返回的type格式化返回
	static void default_callback(redisAsyncContext *c, void *r, void *privdata);
	// 当返回的array要按顺序成对格式化成key=>value格式的时候使用此回调
	static void return_pair_callback(redisAsyncContext *c, void *r, void *privdata);
	// 当需要把请求参数保存下来作为array的key，返回的array当成value的时候使用此回调
	static void arg_key_callback(redisAsyncContext *c, void *r, void *privdata);
	static void quit_callback(redisAsyncContext *c, void *r, void *privdata);
	static void subscribe_callback(redisAsyncContext *c, void *r, void *privdata);
	void command_arg_key(const char* cmd, php::parameters& params);
	void command(const char* cmd, redisCallbackFn *fn = default_callback, void *privdata = NULL);
	void command(const char* cmd, const char* arg, redisCallbackFn *fn = default_callback, void *privdata = NULL);
	void command(int argc, const char **argv, const size_t *argvlen, redisCallbackFn *fn = default_callback, void *privdata = NULL);
	void connect(php::array& arr);
	void close();
	void set_result(php::value str) { result_ = str; };
	php::value& GetResult() { return result_; };
	void set_error(php::value str) { error_ = str; };
	void default_set_result(void *r, void *privdata);
	void continue_get_result();
	redisAsyncContext *context_;
	php::value result_;
	php::value error_;
	redis_cmd_cache cache_;
};

}
}
