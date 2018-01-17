#pragma once

namespace flame {
namespace net {
namespace http {
	class server_connection: public server_connection_base {
	public:
		server_connection(void* ptr);
		virtual ~server_connection();
		virtual ssize_t parse(const char* data, ssize_t size) override;
		virtual void close() override;
	private:
		http_parser          hpp_;
		http_parser_settings hps_;
		static int hp_url_cb(http_parser* parser, const char* at, size_t length);
		static int hp_header_field_cb(http_parser* parser, const char* at, size_t length);
		static int hp_header_value_cb(http_parser* parser, const char* at, size_t length);
		void header_add();
		static int hp_header_complete_cb(http_parser* parser);
		static int hp_body_cb(http_parser* parser, const char* at, size_t length);
		static int hp_message_complete(http_parser* parser);

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
