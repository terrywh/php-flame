#pragma once

namespace flame {
namespace http {
	class client_request: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params);
		php::value to_string(php::parameters& params);
	private:
		void build_ex();
		std::shared_ptr<php::url> url_;
		boost::beast::http::message<true, value_body<true>> ctr_;
		template <class Stream>
		friend class executor;
	};
}
}
