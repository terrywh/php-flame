#pragma once

namespace net {
	class tcp_server: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		tcp_server();
		~tcp_server();
		php::value __destruct(php::parameters& params);
		php::value listen(php::parameters& params);
		php::value local_addr(php::parameters& params);
		php::value local_port(php::parameters& params);
		php::value accept(php::parameters& params);
		php::value close(php::parameters& params);
	private:
		struct evconnlistener*   listener_;
		bool              binded_;
		// 兼容 v4/v6 地址
		union {
			sockaddr     va;
			sockaddr_in  v4;
			sockaddr_in6 v6;
		} local_addr_;
		php::callable     cb_;
		static void error_handler(struct evconnlistener *lis, void *ptr);
		static void accept_handler(struct evconnlistener *listener,
			evutil_socket_t fd, struct sockaddr *addr, int len, void *ptr);
	};
}
