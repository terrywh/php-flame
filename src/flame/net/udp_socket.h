#pragma once

namespace flame {
namespace net {
	class udp_socket: public php::class_base {
	public:
		udp_socket();
		php::value bind(php::parameters& params);
		// 需要有额外的一个引用参数
		php::value recv_from(php::parameters& params);
		// 发送数据 + 目标地址
		php::value send_to(php::parameters& params);
		php::value close(php::parameters& params);
		// property local_address ""
	private:
		uv_udp_t    socket_;
		php::buffer buffer_;
		php::value* addr_;
		php::value* port_;
		static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags);
		static void send_cb(uv_udp_send_t* req, int status);
		static void close_cb(uv_handle_t* handle);
	};
}
}
