#include <phpext.h>
#include <typeinfo>
#include <iostream>
#include <curl.h>
#include "../fiber.h"



// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);
namespace flame {
namespace net {
size_t write_callback(char* ptr, size_t size, size_t nmemb, void *userdata);

struct http_request: public php::class_base {
	http_request() {
	}
	virtual ~http_request() {
	}
	php::value __construct(php::parameters& params);
	php::value __destruct(php::parameters& params) {}
	// 设置header
	//php::value header(php::parameters& params);
	// 设置post数据
	php::value content(php::parameters& params) {
		php::string& content = params[0];
		_postfield = content;
		return nullptr;
	}

	php::string get_method() {
		php::value& p = prop("method");
		return p;
	}
	php::array get_header() {
		php::value p = prop("header");
		return p;
	}
	int get_timeout() {
		php::value& p = prop("timeout");
		if (p.is_null()) {
			return 10;
		} else {
			return p;
		}
	}
	php::string _postfield;
};

class http_client: public php::class_base {
public:
	http_client():_curlm_handle(NULL),_exe_times(0),_debug(0){
	}
	virtual ~http_client() {
		Release();
	}
	// curl要用的回调
	static int handle_socket(CURL* easy, curl_socket_t s, int action, void *userp, void *socketp);
	static int start_timeout(CURLM* multi, long timeout_ms, void* userp);
	static void curl_perform(uv_poll_t *req, int status, int events);
	
	php::value __construct(php::parameters& params);
	// 执行
	php::value exec(php::parameters& params);

	php::value debug(php::parameters& params);

	//不暴露给php的内部函数

	void SetResult(const char* p, int len) {
		if (_result.is_empty() == false) {
			php::string buf(_result.length() + len);
			memcpy(buf.data(),_result.data(), _result.length());
			memcpy(buf.data()+_result.length(), p, len);
			_result = buf;
		}
		else {
			_result = php::string(p,len);
		}
	}
	php::value exec(http_request* request);
	CURLM* get_curl_handle() {
		return _curlm_handle;
	}
	int ExeOnce() {
		return _exe_times++;
	}
	void Release() {
		if (_curlm_handle) {
			curl_multi_cleanup(_curlm_handle);
			_curlm_handle = NULL;
		}
	}

	uv_timer_t _timeout;
	flame::fiber* _fiber;
	php::string _result;
private:
	CURLM* _curlm_handle;
	int _exe_times;
	int _debug;
};

typedef struct curl_context_s {
	uv_poll_t poll_handle;
	curl_socket_t sockfd;
	http_client* client;

} curl_context_t;

php::value http_get(php::parameters& params);
php::value http_post(php::parameters& params);
php::value http_put(php::parameters& params);

}
}
