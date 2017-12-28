#pragma once

namespace flame{
namespace net {
namespace http {
	// class handler;
	// class server_connection;
	class server_response: public php::class_base {
	public:
		~server_response();
		// 声明 ZEND_ACC_PRIVATE 禁止手动创建
	 	php::value __construct(php::parameters& params) {
	 		return nullptr;
		}
		// property status integer
		// property header array
		php::value set_cookie(php::parameters& params);
		php::value write_header(php::parameters& params);
		php::value write(php::parameters& params);
		php::value end(php::parameters& params);
	private:
		void buffer_header();
		void buffer_body(const char* data, unsigned short size);
		php::value write_buffer(php::parameters& params);

		void init(server_connection* conn);

		server_connection*                   conn_;
		php::buffer                        buffer_;
		std::map<std::string, php::string> cookie_;
		bool is_head_sent;
		bool is_body_sent;
		bool is_chunked;

		friend class server_connection;
	};
}
}
}
