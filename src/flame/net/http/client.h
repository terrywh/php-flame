#include "../../fiber.h"



// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);
namespace flame {
namespace net {
namespace http {
size_t write_callback(char* ptr, size_t size, size_t nmemb, void *userdata);

struct request: public php::class_base {
	request() {
	}
	virtual ~request() {
	}
	php::value __construct(php::parameters& params);
	php::value __destruct(php::parameters& params) {}
	// 设置header
	//php::value header(php::parameters& params);
	// 设置post数据
	php::value content(php::parameters& params) {
		php::string& content = params[0];
		postfield_ = content;
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
	int gettimeout_() {
		php::value& p = prop("timeout");
		if (p.is_null()) {
			return 10;
		} else {
			return p;
		}
	}
	php::string postfield_;
};

class client: public php::class_base {
public:
	client():curlm_handle_(NULL),exe_times_(0),debug_(0){
	}
	virtual ~client() {
		Release();
	}
	// curl要用的回调
	static int handle_socket(CURL* easy, curl_socket_t s, int action, void *userp, void *socketp);
	static int starttimeout_(CURLM* multi, long timeout_ms, void* userp);
	static void curl_perform(uv_poll_t *req, int status, int events);
	
	php::value __construct(php::parameters& params);
	// 执行
	php::value exec(php::parameters& params);

	php::value debug(php::parameters& params);

	//不暴露给php的内部函数

	void SetResult(const char* p, int len) {
		if (result_.is_empty() == false) {
			php::string buf(result_.length() + len);
			memcpy(buf.data(),result_.data(), result_.length());
			memcpy(buf.data()+result_.length(), p, len);
			result_ = buf;
		}
		else {
			result_ = php::string(p,len);
		}
	}
	php::value exec(request* request);
	CURLM* get_curl_handle() {
		return curlm_handle_;
	}
	int ExeOnce() {
		return exe_times_++;
	}
	void Release() {
		if (curlm_handle_) {
			curl_multi_cleanup(curlm_handle_);
			curlm_handle_ = NULL;
		}
	}

	uv_timer_t timeout_;
	flame::fiber* fiber_;
	php::string result_;
private:
	CURLM* curlm_handle_;
	int exe_times_;
	int debug_;
};

typedef struct curl_context_s {
	uv_poll_t poll_handle;
	curl_socket_t sockfd;
	client* cli;

} curl_context_t;

php::value get(php::parameters& params);
php::value post(php::parameters& params);
php::value put(php::parameters& params);

}
}
}
