#pragma once

namespace net { namespace http {
	class client_request;

	class client: public php::class_base {
	public:
		struct connection_wrapper {
			int                ttl;
			evhttp_connection* conn;
		};
		static void init(php::extension_entry& extension);
		client();
		~client();
		php::value __construct(php::parameters& params);
		// 简单 GET 请求
		php::value get(php::parameters& params);
		// 其他请求
		php::value execute(php::parameters& params);

		evhttp_connection* acquire(const std::string& key);
		void release(const std::string& key, evhttp_connection* conn);
	private:
		static void sweep_handler(evutil_socket_t fd, short events, void* ctx);
		std::multimap<std::string, connection_wrapper> connection_;
		event ev_;
		int   ttl_;
	};
}}
