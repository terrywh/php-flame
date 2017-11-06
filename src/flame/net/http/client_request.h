#pragma once

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);
namespace flame {
class coroutine;
namespace net {
namespace http {
	class client;
	class client_response;
	class client_request: public php::class_base {
	public:
		client_request();
		~client_request() {
			close();
		}
		php::value __construct(php::parameters& params);
		php::value ssl(php::parameters& params);
	private:
		void        close();
		void        build(client* cli);
		curl_slist* build_header();

		static size_t read_cb(void *ptr, size_t size, size_t nmemb, void *stream);
		static size_t body_cb(char* ptr, size_t size, size_t nmemb, void *userdata);
		static size_t head_cb(char* ptr, size_t size, size_t nitems, void* userdata);
		bool          done_cb(CURLMsg* msg);

		CURL*         curl_;
		curl_slist*   curl_header;
		curl_socket_t curl_fd;
		uv_poll_t*    poll_;
		client*        cli_;
		coroutine*        co_;
		php::value       ref_;
		client_response* res_;
		php::object      res_obj;

		friend class client;
	};
}
}
}
