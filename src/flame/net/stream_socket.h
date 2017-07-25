#pragma once 

namespace flame {
namespace net {
	class stream_socket: public php::class_base {
	public:
		stream_socket(uv_stream_t* s);
		php::value __destruct(php::parameters& params);
		php::value read(php::parameters& params);
		php::value write(php::parameters& params);
		php::value close(php::parameters& params);
	private:
		uv_stream_t* pstream_;
		php::buffer  rbuffer_;
		php::string  wbuffer_;
		static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
		static void write_cb(uv_write_t* req, int status);
		static void close_cb(uv_handle_t* handle);

		friend class stream_server;
	};
}
}
