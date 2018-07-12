#pragma once

namespace flame {
namespace http {
	class handler;
	class server_request: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params);
		php::value to_string(php::parameters& params);
		// virtual ~server_request();
	private:
		std::shared_ptr<handler> handler_;
		void build_ex(const boost::beast::http::message<true, value_body<true>>& ctr);

		friend class handler;
	};
}
}
