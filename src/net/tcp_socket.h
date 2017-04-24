#pragma once

namespace net {
	class tcp_server;
	class tcp_socket: public php::class_base {
	public:
		tcp_socket():closed_(true) {}
		// 通过指定连接地址构造一个 tcp_socket，支持指定超时时间
		php::value __construct(php::parameters& params);
		php::value __destruct(php::parameters& params);
		php::value remote_addr(php::parameters& params);
		// recv 支持两种形式：
		// 1. 接收指定大小
		// 2. 接收指定任意停止符
		// 两种形式都支持指定超时
		php::value recv(php::parameters& params);
		php::value send(php::parameters& params);
		php::value flush(php::parameters& params);
		php::value send_now(php::parameters& params);
		php::value close(php::parameters& params);
	private:
		php::value recv_length(int length, std::int64_t dead);
		php::value recv_delims(zend_string* delims, std::int64_t dead);
		mill_tcpsock socket_;
		bool         closed_;
		mill_ipaddr remote_addr_;
		friend class tcp_server;
	};
}
