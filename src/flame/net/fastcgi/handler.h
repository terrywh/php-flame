#pragma once

namespace flame {
namespace net {
namespace fastcgi {

	class handler: public php::class_base {
	public:
		handler();
		php::value __invoke(php::parameters& params);
		php::value put(php::parameters& params);
		php::value remove(php::parameters& params);
		php::value post(php::parameters& params);
		php::value get(php::parameters& params);
		php::value handle(php::parameters& params);
		void on_request(php::object& req, php::object& res);
	private:
		std::map<std::string, php::callable> handle_put;
		std::map<std::string, php::callable> handle_delete;
		std::map<std::string, php::callable> handle_post;
		std::map<std::string, php::callable> handle_get;
		php::callable                        handle_def;
	};
}
}
}
