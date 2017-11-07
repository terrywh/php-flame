#pragma once

namespace flame {
class coroutine;
namespace net {
	class udp_socket: public php::class_base {
	public:
		udp_socket();
		~udp_socket();
		php::value bind(php::parameters& params);
		// 需要有额外的一个引用参数
		php::value recv(php::parameters& params);
		// 发送数据 + 目标地址
		php::value send(php::parameters& params);
		php::value close(php::parameters& params);
		void close(bool stop_recv);
		// property local_address ""

		uv_udp_t*      stream; // 由于异步关闭需要等待 close_cb 才能进行回收
	private:

		coroutine*     cor_; // 读取协程
		php::value     ref_; // 对象引用

		php::string    rbuffer_;
		php::value*    addr_;
		php::value*    port_;

		static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags);
		static void send_cb(uv_udp_send_t* req, int status);
	};
}
}
