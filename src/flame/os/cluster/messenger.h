#pragma once

namespace flame {
namespace os {
namespace cluster {
	class messenger {
	public:
		messenger();
		~messenger();
		void start();
		php::callable cb_string;
		php::callable cb_socket;
		int           cb_type;
		void send(php::parameters& params, uv_stream_t* stream = nullptr);
	private:
		// 生命周期需与 PHP 对象不符，故使用动态分配
		uv_pipe_t*  pipe_;
		int         stat_;
		short       size_;
		php::buffer data_;
		static void alloc_cb(uv_handle_t* handle, size_t suggest, uv_buf_t* buf);
		static void read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);
		static void send_string_cb(uv_write_t* handle, int status);
		static void send_socket_cb(uv_write_t* handle, int status);
		size_t parse(char* data, size_t size);
		enum {
			PPS_SIZE_0,
			PPS_SIZE_1,
			PPS_DATA,
		};
		void on_socket();
		void on_string();
	};
}
}
}
