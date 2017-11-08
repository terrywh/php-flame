#pragma once

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);
namespace flame {
class coroutine;
namespace net {
namespace http {
	class client_request: public php::class_base {
	public:
		client_request();
		~client_request() {
			close();
		}
		php::value __construct(php::parameters& params);
		php::value ssl(php::parameters& params);
	private:

		void build();
		void build_header();
		void build_cookie();
		void build_option();
		void close();
		void done_cb(CURLMsg* msg);

		static size_t body_read_cb(void *ptr, size_t size, size_t nmemb, void *userdata);

		CURL*         easy_;
		curl_slist*   dns_;
		curl_slist*   header_;

		char*         body_;
		size_t        size_;
		friend class client;
	};
}
}
}
