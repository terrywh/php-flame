#pragma once

namespace net {
	class udp_socket: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		udp_socket();
		~udp_socket();
		php::value __destruct(php::parameters& params);
		php::value connect(php::parameters& params);
		php::value remote_addr(php::parameters& params);
		php::value remote_port(php::parameters& params);
		php::value local_addr(php::parameters& params);
		php::value local_port(php::parameters& params);
		php::value close(php::parameters& params);
		php::value read(php::parameters& params);
		php::value write2(php::parameters& params);
		php::value write(php::parameters& params);

		php::value bind(php::parameters& params);
    private:
        struct addrinfo* resolver(const char* host, const char* port, int sock_type);
	protected:
		evutil_socket_t socket_;
		zend_string*    rbuffer_;
		zend_string*    wbuffer_;
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
		bool            connected_;
		bool            binded_;

		event           ev_;
		php::callable   cb_;
		static void read_handler(evutil_socket_t fd, short events, void* data);
		static void write_handler(evutil_socket_t fd, short events, void* data);
		static void write2handler(evutil_socket_t fd, short events, void* data);
	};
}
