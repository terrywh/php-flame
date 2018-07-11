#pragma once

namespace flame {
namespace http {
	class handler;
	class server_response: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		// 声明为 ZEND_ACC_PRIVATE 禁止创建（不会被调用）
		php::value __construct(php::parameters& params);
		php::value to_string(php::parameters& params);
		php::value set_cookie(php::parameters& params);
		php::value write_header(php::parameters& params);
		php::value write(php::parameters& params);
		php::value end(php::parameters& params);
		php::value file(php::parameters& params);
		server_response();
	private:
		boost::beast::http::message<false, value_body<false>> ctr_;
		boost::beast::http::serializer<false, value_body<false>> sr_;

		int status_;
		enum {
			STATUS_ALREADY_BUILT = 0x01,
			STATUS_HEADER_SENT   = 0x02,
			STATUS_RESPONSE_END  = 0x04,
		};
		void build_ex();
		std::shared_ptr<handler> handler_;

		friend class handler;
		friend class chunked_writer;
		friend class file_writer;
	};
}
}
