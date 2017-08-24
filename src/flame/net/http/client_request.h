#pragma once

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);
namespace flame {
namespace net {
namespace http {
	class client;
	class client_request: public php::class_base {
	public:
		client_request();
		~client_request() {
			release();
		}
		php::value __construct(php::parameters& params);
		void release();
		void build(client* cli);
		php::string& parse_body(php::array& arr);
		curl_slist*  build_header();

		CURL*                       curl_;
		curl_slist*                slist_;
		uv_poll_t            poll_handle_;
		curl_socket_t             sockfd_;
		client*                      cli_;
		std::function<void(CURLMsg*)> cb_;
		php::string               result_;
	};
}
}
}
