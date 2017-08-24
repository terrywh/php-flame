#pragma once

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);
namespace flame {
namespace net {
namespace http {

class client: public php::class_base {
public:
	client();
	~client() {
		release();
	}
	php::value __construct(php::parameters& params);
	// curl要用的回调
	static int handle_socket(CURL* easy, curl_socket_t s, int action, void *userp, void *socketp);
	static int start_timeout(CURLM* multi, long timeout_ms, void* userp);
	static void curl_perform(uv_poll_t *req, int status, int events);
	// 执行
	php::value exec(php::parameters& params);
	php::value exec(php::object& request);
	php::value debug(php::parameters& params);
	CURLM* get_curl_handle();
	void release();
	uv_timer_t timeout_;
private:

	CURLM* curlm_handle_;
	int debug_;
};

php::value get(php::parameters& params);
php::value post(php::parameters& params);
php::value put(php::parameters& params);

}
}
}
