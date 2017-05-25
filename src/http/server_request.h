#pragma once

namespace http {
	class server;
	class server_response;
	class server_request: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		void init(evhttp_request* evreq, server* svr);
		php::value body(php::parameters& params);
		friend class server;
	private:
		struct evhttp_request* req_;
		friend class server;
		friend class server_response;
	};
}
