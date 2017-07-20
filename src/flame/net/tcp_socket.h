#pragma once 

namespace flame {
namespace net {
	class tcp_socket: public php::class_base {
	public:
		tcp_socket();
		php::value __destruct(php::parameters& params);
		php::value connect(php::parameters& params);
		php::value read(php::parameters& params);
		php::value write(php::parameters& params);
		php::value close(php::parameters& params);
		// property local_address ""
		// property local_port 0
		// property remote_address ""
		// property remote_port 0
		uv_stream_t* stream();
	private:
		uv_tcp_t     socket_;
		static void connect_cb(uv_connect_t* req, int status);
		php::buffer  rbuffer_;
		php::string  wbuffer_;
		static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
		static void write_cb(uv_write_t* req, int status);
		static void close_cb(uv_handle_t* handle);
		void init_prop();

		friend class tcp_server;
	};
}
}
