#pragma once

namespace http {
	class client;
	class client_request;
	class client_response: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		void init(evhttp_request* req);
		client_response();
		~client_response();
		// 方便起见，client_response 对象能够直接作为文本使用（响应体）
		php::value __toString(php::parameters& params);
	private:
		bool keepalive();
		evhttp_request* req_;
		bool keepalive_;
		friend class client;
		friend class client_request;
	};
}
