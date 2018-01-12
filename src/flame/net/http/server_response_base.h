#pragma once

namespace flame{
namespace net {
namespace http {

	class server_response_base: public php::class_base {
	public:
		virtual ~server_response_base() {};
		// property status integer
		// property header array
		php::value set_cookie(php::parameters& params);
		php::value write_header(php::parameters& params);
		php::value write(php::parameters& params);
		php::value end(php::parameters& params);
	protected:
		virtual void init(server_connection_base* conn);
		virtual void buffer_header() = 0;
		virtual void buffer_body(const char* data, unsigned short size) = 0;
		virtual void buffer_ending();
		php::value write_buffer(php::parameters& params);

		server_connection_base*              conn_;
		php::buffer                        buffer_;
		std::map<std::string, php::string> cookie_;
		bool is_head_sent;
		bool is_body_sent;

		friend class server_connection;
	};
}
}
}
