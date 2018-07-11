#pragma once

namespace flame {
namespace http {
	class server_request: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params);
		php::value to_string(php::parameters& params);
		// virtual ~server_request();
	private:
		void build_ex();
		std::shared_ptr<php::url> url_;
		boost::beast::http::message<true, value_body<true>> ctr_;

		bool match_;
		
		friend class handler;
	};
}
}
