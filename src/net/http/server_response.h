#pragma once

namespace net { namespace http {
	class server;
	class server_response: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		server_response();
		~server_response();
		// response 对象销毁时结束请求
		php::value __destruct(php::parameters& params);
		php::value end(php::parameters& params);
	private:
		static void complete_handler(struct evhttp_request* req_, void* ctx);
		bool                   completed_;
		struct evhttp_request* req_;
		struct evbuffer*       wbuffer_;
		std::list<php::string> wbuffer_cache_;
		php::callable          cb_;
		friend class server;
	};
}}
