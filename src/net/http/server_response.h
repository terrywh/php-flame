#pragma once

namespace net { namespace http {
	class server;
	class server_request;
	class header;
	class server_response: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		void init(evhttp_request* evreq, server* svr);
		server_response();
		~server_response();
		// response 对象销毁时结束请求
		php::value __destruct(php::parameters& params);
		// response 对象倾向于使用 transfer-encoding: chunked；
		// 否则请当指定 content-length 头并自行填充长度信息
		php::value write_header(php::parameters& params);
		php::value write(php::parameters& params);
		php::value end(php::parameters& params);
	private:
		static void complete_handler(struct evhttp_request* req_, void* ctx);
		bool                   header_sent_;
		bool                   completed_;
		struct evhttp_request* req_;
		evbuffer*              chunk_;
		std::list<php::string> wbuffer_;
		php::callable          cb_;
		server*                svr_;
		friend class server;
		friend class server_request;
	};
}}
