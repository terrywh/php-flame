#pragma once

namespace flame{
namespace net {
namespace fastcgi {
	class server_connection;
	class server_response: public php::class_base {
	public:
		server_response();
		~server_response();
		typedef struct {
			unsigned char  version;
			unsigned char  type;
			unsigned short request_id;
			unsigned short content_length;
			unsigned char  padding_length;
			unsigned char  reserved;
		} record_header_t;
		// property status integer
		// property header array
		// property header_sent boolean
		php::value write_header(php::parameters& params);
		php::value write(php::parameters& params);
		php::value end(php::parameters& params);
	private:
		void buffer_head();
		void buffer_body(const char* data, unsigned short size);
		void buffer_ending();
		void buffer_write();
		server_connection* conn_;
		record_header_t  header_;
		php::buffer      buffer_;
		static void write_cb(uv_write_t* req, int status);
		friend class server;
		friend class server_connection;
	};
}
}
}
