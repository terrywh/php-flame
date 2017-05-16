#pragma once

namespace net {
	class tcp_server;
	class tcp_socket: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		tcp_socket(bool connected = false);
		php::value __construct(php::parameters& params);
		php::value __destruct(php::parameters& params);
		php::value connect(php::parameters& params);
		php::value remote_addr(php::parameters& params);
		php::value remote_port(php::parameters& params);
		php::value local_addr(php::parameters& params);
		php::value local_port(php::parameters& params);
		php::value close(php::parameters& params);
		php::value read(php::parameters& params);
		php::value write(php::parameters& params);
	private:
		static void event_handler(struct bufferevent* bev, short events, void *ctx);
		static void write_handler(struct bufferevent *bev, void *ctx);
		static void read_handler(struct bufferevent *bev, void *ctx);


		bufferevent* socket_;
		// 兼容 v4/v6 地址
		union {
			sockaddr     va;
			sockaddr_in  v4;
			sockaddr_in6 v6;
		} local_addr_;
		union {
			sockaddr     va;
			sockaddr_in  v4;
			sockaddr_in6 v6;
		} remote_addr_;
		bool connected_;
		php::callable cb_;
		// std::list<php::callable> cb_;
		// read helper
		zend_string* delim_;
		ssize_t length_;
		// write helper
		zend_string* wbuffer_;
		friend class tcp_server;
	};
}
