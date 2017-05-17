#pragma once

namespace net { namespace http {
	class server;
	class server_request: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		friend class server;
	private:
		struct evhttp_request* req_;
	};
}}
