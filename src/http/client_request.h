#pragma once

namespace http {
	class client;
	class client_response;
	class header;
	class client_request: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		client_request();
		~client_request();
		php::value __construct(php::parameters& parmas);
	private:
		int execute(client* cli);
		evhttp_request* req_;
		evhttp_cmd_type cmd_;
		evhttp_uri*     uri_;
		client*         cli_;
		std::string     key_;
		php::callable   cb_;
		evhttp_connection* conn_;
		friend class client;
		friend class client_response;
	};
}
