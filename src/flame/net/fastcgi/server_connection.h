#pragma once

namespace flame {
namespace net {
namespace http {
	class server_request;
}
namespace fastcgi {
	class server;
	class server_connection {
	private:
		union {
			uv_stream_t socket_;
			uv_pipe_t socket_pipe_;
			uv_tcp_t  socket_tcp_;
		};
		server*                   svr_;
		fastcgi_parser            fpp_;
		fastcgi_parser_settings   fps_;
		
		void start();
		void close();
		char buffer_[8192];
		static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
		static void close_cb(uv_handle_t* handle);

		static int fp_begin_request_cb(fastcgi_parser* parser);
		static int fp_param_key_cb(fastcgi_parser* parser, const char* at, size_t length);
		static int fp_param_val_cb(fastcgi_parser* parser, const char* at, size_t length);
		static int fp_end_param_cb(fastcgi_parser* parser);
		static int fp_data_cb(fastcgi_parser* parser, const char* at, size_t length);
		static int fp_end_data_cb(fastcgi_parser* parser);
		static int fp_end_request_cb(fastcgi_parser* parser);

		static int mp_key_cb(multipart_parser* parser, const char *at, size_t length);
		static int mp_val_cb(multipart_parser* parser, const char *at, size_t length);
		static int mp_dat_cb(multipart_parser* parser, const char *at, size_t length);

		static int kv_key_cb(kv_parser* parser, const char* at, size_t length);
		static int kv_val_cb_1(kv_parser* parser, const char* at, size_t length);
		static int kv_val_cb_2(kv_parser* parser, const char* at, size_t length);

		int parse(const char* data, int size);

		php::buffer key_;
		php::buffer val_;
		php::object req_;
		php::object res_;
		php::array* ctr_;
	public:

		friend class server;
		friend class server_response;
	};
}
}
}
