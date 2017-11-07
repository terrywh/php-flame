#pragma once

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);
namespace flame {
namespace net {
namespace http {
	class client_request;
	class client_response;
	class client: public php::class_base {
	public:
		client();
		~client() {
			destroy();
		}
		php::value __construct(php::parameters& params);
		void destroy();
		// 执行
		php::value exec1(php::parameters& params);
		php::value exec2(php::object& request);
		php::value debug(php::parameters& params);
	private:
		// curl要用的回调
		static void curl_multi_info_check(client* cli);
		static int  curl_multi_socket_handle(CURL* easy, curl_socket_t s, int action, void *userp, void *socketp);
		static int  curl_multi_timer_handle(CURLM* multi, long timeout_ms, void* userp);
		static void curl_multi_timer_cb(uv_timer_t *req);
		static void curl_multi_socket_poll(uv_poll_t *req, int status, int events);

		static size_t curl_easy_head_cb(char* ptr, size_t size, size_t nitems, void* userdata);
		static size_t curl_easy_body_cb(char* ptr, size_t size, size_t nmemb, void *userdata);
		CURLM*     multi_;
		int        debug_;
		uv_timer_t timer_;

		friend class client_request;
	};
	extern client* default_client;
	php::value get(php::parameters& params);
	php::value post(php::parameters& params);
	php::value put(php::parameters& params);
	php::value remove(php::parameters& params);
}
}
}
