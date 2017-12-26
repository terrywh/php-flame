#pragma once

namespace flame{
namespace net {
namespace http {
	class handler;
	class server_connection;
	class server_response: public php::class_base {
	public:
		// 声明 ZEND_ACC_PRIVATE 禁止手动创建
		php::value __construct(php::parameters& params) {
			return nullptr;
		}
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
		php::value set_cookie(php::parameters& params);
		php::value write_header(php::parameters& params);
		php::value write(php::parameters& params);
		php::value end(php::parameters& params);
	private:
		void buffer_header();
		void buffer_cookie();
		void buffer_body(const char* data, unsigned short size);
		void buffer_ending();
		void write_buffer();
		static void write_cb(uv_write_t* req, int status);
		
		server_connection* conn_;
		record_header_t  header_;
		php::buffer      buffer_;
		std::map<std::string, php::string> cookie_;
		bool header_sent, body_sent;
		friend class handler;
		friend class server_connection;
	};
}
}
}
