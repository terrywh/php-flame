#include <typeinfo>
#include <iostream>
#include <hiredis.h>
#include <async.h>
#include <adapters/libuv.h>
#include <string.h>
#include "../fiber.h"
//#define MY_DEBUG

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);

namespace flame {
namespace db {

struct RedisCmdCache {
	std::vector<std::string> data;
};

class redis_client: public php::class_base {
public:
	redis_client();
	virtual ~redis_client();
	php::value __construct(php::parameters& params);
	php::value __call(php::parameters& params);
	php::value connect(php::parameters& params);
	php::value close(php::parameters& params);
	php::value getlasterror(php::parameters& params)
	{
		if (_error.is_null())
			return php::string();
		else
			return _error;
	};
	php::value hgetall(php::parameters& params);
	php::value hmget(php::parameters& params);
	php::value quit(php::parameters& params);
	php::value subscribe(php::parameters& params);
	
	void SetContext(redisAsyncContext *context) { _context = context; };

	flame::fiber* _fiber;
private:
	// 默认回调，按照redis返回的type格式化返回
	static void DefaultCallback(redisAsyncContext *c, void *r, void *privdata);
	// 当返回的array要按顺序成对格式化成key=>value格式的时候使用此回调
	static void ReturnPairCallback(redisAsyncContext *c, void *r, void *privdata);
	// 当需要把请求参数保存下来作为array的key，返回的array当成value的时候使用此回调
	static void ArgKeyCallback(redisAsyncContext *c, void *r, void *privdata);
	static void QUITCallback(redisAsyncContext *c, void *r, void *privdata);
	static void SUBSCRIBECallback(redisAsyncContext *c, void *r, void *privdata);
	void CommandArgKey(const char* cmd, php::parameters& params);
	void Command(const char* cmd, redisCallbackFn *fn = DefaultCallback, void *privdata = NULL);
	void Command(const char* cmd, const char* arg, redisCallbackFn *fn = DefaultCallback, void *privdata = NULL);
	void Command(int argc, const char **argv, const size_t *argvlen, redisCallbackFn *fn = DefaultCallback, void *privdata = NULL);
	void Connect(php::array& arr);
	void Close();
	void SetResult(php::value str) { _result = str; };
	php::value& GetResult() { return _result; };
	void SetError(php::value str) { _error = str; };
	void DefaultSetResult(void *r, void *privdata);
	void ContinueGetResult();
	redisAsyncContext *_context;
	php::value _result;
	php::value _error;
	RedisCmdCache _cache;
};

}
}
