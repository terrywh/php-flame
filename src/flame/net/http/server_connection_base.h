#pragma once

namespace flame {
namespace net {
namespace http {
	class server_connection_base {
	public:
		// server_connection 对象的生存周期由 C++ 自行控制，不受 PHP 影响 故不需要使用指针形式
		union {
			uv_stream_t sock_;
			uv_pipe_t   sock_uds; // unix domain socket
			uv_tcp_t    sock_tcp;
		};
		server_connection_base(void* ptr);
		
		bool write(const php::string& str, coroutine* co);
		void start();
		virtual ssize_t parse(const char* data, ssize_t size);
		// 回调 server 函数（避免循环模板引用）
		void (*on_request)(const php::object& req, const php::object& res, void* data);
		// void (*on_close)(server_connection_base* conn, void* data);
		void * data;
		void refer() {
			++refer_;
		}
		void close(bool by_response = true);
		bool is_closing;
	private:
		static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void read_cb (uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);
		static void write_cb(uv_write_t* handle, int status);
		static void close_cb(uv_handle_t* handle);
		int  refer_;
	};
}
}
}
