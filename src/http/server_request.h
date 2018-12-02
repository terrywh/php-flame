#pragma once

namespace flame::http {
	class handler;
	class server_request: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params);
		~server_request();
	private:
		void build_ex(const boost::beast::http::message<true, value_body<true>>& ctr);

        friend class _handler;
    };
}
