#pragma once

namespace http {
	class server: public php::class_base {
	public:
		struct handler_wrapper {
			php::callable handler;
			server*       self;
		};
		static void init(php::extension_entry& extension);
		server();
		~server();
		php::value __destruct(php::parameters& params);
		php::value listen_and_serve(php::parameters& params);
		php::value local_addr(php::parameters& params);
		php::value local_port(php::parameters& params);
		php::value handle(php::parameters& params);
		php::value handle_default(php::parameters& params);
		php::value close(php::parameters& params);
		void request_finish();
	private:
		size_t request_count_;
		static void request_handler(struct evhttp_request* req, void* ctx);
		static void error_handler(struct evconnlistener *lis, void *ptr);
		// 兼容 v4/v6 地址
		union {
			sockaddr     va;
			sockaddr_in  v4;
			sockaddr_in6 v6;
		} local_addr_;
		struct evhttp*              server_;
		struct evhttp_bound_socket* socket_;
		bool                        closed_;
		handler_wrapper             handler_default_;
		std::list<handler_wrapper>  handler_;
		php::callable               cb_;
	};
}
