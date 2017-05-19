#pragma once

namespace net { namespace http {
	class server: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		server();
		~server();
		php::value __destruct(php::parameters& params);
		php::value listen_and_serve(php::parameters& params);
		php::value local_addr(php::parameters& params);
		php::value local_port(php::parameters& params);
		php::value handle(php::parameters& params);
		php::value close(php::parameters& params);
	private:

		static void request_handler(struct evhttp_request* req, void* ctx);
		static void error_handler(struct evconnlistener *lis, void *ptr);
		// 兼容 v4/v6 地址
		union {
			sockaddr     va;
			sockaddr_in  v4;
			sockaddr_in6 v6;
		} local_addr_;
		struct evhttp*           server_;
		php::callable            handler_default_;
		std::list<php::callable> handler_;
		php::callable            cb_;
	};
}}
