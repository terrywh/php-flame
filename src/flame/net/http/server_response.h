#pragma once

namespace flame{
namespace net {
namespace http {
	// class handler;
	// class server_connection;
	class server_response: public server_response_base {
	public:
		// server_response();
		virtual ~server_response();
		// 声明 ZEND_ACC_PRIVATE 禁止手动创建
	 	php::value __construct(php::parameters& params) {
	 		return nullptr;
		}
		// property status integer
		// property header array
		php::value set_cookie(php::parameters& params) {
			return server_response_base::set_cookie(params);
		}
		php::value write_header(php::parameters& params) {
			return server_response_base::write_header(params);
		}
		php::value write(php::parameters& params) {
			return server_response_base::write(params);
		}
		php::value end(php::parameters& params) {
			return server_response_base::end(params);
		}
	private:
		virtual void buffer_header() override;
		virtual void buffer_body(const char* data, unsigned short size) override;

		virtual void init(server_connection_base* conn) override;
		bool is_chunked;

		friend class server_connection;
	};
}
}
}
