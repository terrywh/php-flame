#pragma once

namespace flame {
namespace net {
namespace fastcgi {
	class server_connection: public http::server_connection_base {
	public:
		server_connection(void* ptr);
		virtual ~server_connection();
		virtual ssize_t parse(const char* data, ssize_t size) override;
		virtual void close() override;
	private:
		fastcgi_parser          fpp_;
		fastcgi_parser_settings fps_;
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

		const char* key_data;
		size_t      key_size;

		php::buffer key_;
		php::buffer val_;

		php::array  query_;
		php::array  header_;
		php::array  cookie_;
		php::array  body_;
		php::array  body_item;

		friend class server_response;
	};
}
}
}
