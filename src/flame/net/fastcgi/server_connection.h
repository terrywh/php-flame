#pragma once

#include "../http/server_handler.h"

namespace flame {
namespace net {
namespace fastcgi {
	class server_connection;
	typedef http::server_handler<server_connection> handler_t;

	class server_connection {
	private:

		handler_t*              svr_;
		fastcgi_parser          fpp_;
		fastcgi_parser_settings fps_;

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
		static int mp_dat_end(multipart_parser* parser);

		static int kv_key_cb(kv_parser* parser, const char* at, size_t length);

		static int header_val_cb(kv_parser* parser, const char* at, size_t length);
		static int query_val_cb (kv_parser* parser, const char* at, size_t length);
		static int cookie_val_cb(kv_parser* parser, const char* at, size_t length);
		static int body_val_cb  (kv_parser* parser, const char* at, size_t length);

		int parse(const char* data, int size);


		const char* key_data;
		size_t      key_size;

		php::buffer key_;
		php::buffer val_;

		php::object req_;
		php::object res_;

		php::array  query_;
		php::array  header_;
		php::array  cookie_;
		php::array  body_;
		php::array  body_item;
	public:
		// server_connection 对象的生存周期由 C++ 自行控制，不受 PHP 影响
		// 故不需要使用指针形式
		union {
			uv_stream_t socket_;
			uv_pipe_t   socket_pipe;
			uv_tcp_t    socket_tcp;
		};
		server_connection(handler_t* svr);
		void start();
		void close();
		friend class server_response;
	};
}
}
}
