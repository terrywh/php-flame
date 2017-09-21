//#define MY_DEBUG

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);

namespace flame {
namespace db {

class redis: public php::class_base {
public:
	typedef struct {
		redis*                  self;
		flame::fiber*          fiber;
		std::vector<php::string> key;
		php::callable             cb;
	} redisRequest; // 由于特殊使用方式，这里借鉴 hiredis 命名

	redis();
	virtual ~redis();
	php::value connect(php::parameters& params);
	php::value close(php::parameters& params);

	php::value __call(php::parameters& params);
	php::value hgetall(php::parameters& params);
	php::value hmget(php::parameters& params);
	php::value mget(php::parameters& params);
	php::value quit(php::parameters& params);
	php::value subscribe(php::parameters& params);
private:
	redisAsyncContext *context_;

	php::string host_;
	int         port_;
	uv_timer_t  connect_interval;
	void connect();
	static void cb_connect_interval(uv_timer_t* tm);
	void close();
	void command(const php::string& cmd, php::parameters& params, int start, int offset, redisCallbackFn* fn, const php::value& cb = nullptr);

	static void cb_connect(const redisAsyncContext *c, int status);
	static void cb_disconnect(const redisAsyncContext *c, int status);
	static void cb_dummy(redisAsyncContext *c, void *r, void *privdata);
	// 默认回调，按照redis返回的type格式化返回
	static void cb_default(redisAsyncContext *c, void *r, void *privdata);
	// 当返回的array要按顺序成对格式化成key=>value格式的时候使用此回调
	static void cb_assoc_1(redisAsyncContext *c, void *r, void *privdata);
	// 当需要把请求参数保存下来作为array的key，返回的array当成value的时候使用此回调
	static void cb_assoc_2(redisAsyncContext *c, void *r, void *privdata);
	// hmget专用，因为第二个参数是hash，有点特殊
	// static void cb_hmget(redisAsyncContext *c, void *r, void *privdata);
	static void cb_quit(redisAsyncContext *c, void *r, void *privdata);
	static void cb_subscribe(redisAsyncContext *c, void *r, void *privdata);

};

}
}
