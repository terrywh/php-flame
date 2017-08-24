#include "http.h"
#include "client.h"
#include "client_request.h"
#include "server_request.h"

namespace flame {
namespace net {
namespace http {
	void init(php::extension_entry& ext) {
		// curl init
		if(curl_global_init(CURL_GLOBAL_ALL)) {
			php::exception("curl_glbal_init fail", -1);
		}
		// shortcut for class_client
		ext.add<::flame::net::http::get>("flame\\net\\http\\get");
		ext.add<::flame::net::http::post>("flame\\net\\http\\post");
		ext.add<::flame::net::http::put>("flame\\net\\http\\put");
		// class client_request
		php::class_entry<client_request> class_client_request("flame\\net\\http\\client_request");
		class_client_request.add(php::property_entry("url", ""));
		class_client_request.add(php::property_entry("method", ""));
		class_client_request.add(php::property_entry("timeout", 10));
		class_client_request.add(php::property_entry("header", nullptr));
		class_client_request.add(php::property_entry("body", nullptr));
		class_client_request.add<&client_request::__construct>("__construct");
		ext.add(std::move(class_client_request));
		// class client
		php::class_entry<client> class_client("flame\\net\\http\\client");
		class_client.add<&client::__construct>("__construct");
		class_client.add<&client::exec>("exec");
		ext.add(std::move(class_client));
		// class server_request
		php::class_entry<server_request> class_server_request("flame\\net\\http\\server_request");
		class_server_request.add(php::property_entry("method", ""));
		class_server_request.add(php::property_entry("uri", ""));
		class_server_request.add(php::property_entry("query", nullptr));
		class_server_request.add(php::property_entry("header", nullptr));
		class_server_request.add(php::property_entry("cookie", nullptr));
		class_server_request.add(php::property_entry("body", nullptr));
		ext.add(std::move(class_server_request));
	}
	// 下面 HTTP 状态定义来自 nodejs/http_parser
	std::map<int, std::string> status_mapper {
		{100, "Continue"},
		{101, "Switching Protocols"},
		{102, "Processing"},
		{200, "OK"},
		{201, "Created"},
		{202, "Accepted"},
		{203, "Non-Authoritative Information"},
		{204, "No Content"},
		{205, "Reset Content"},
		{206, "Partial Content"},
		{207, "Multi-Status"},
		{208, "Already Reported"},
		{226, "IM Used"},
		{300, "Multiple Choices"},
		{301, "Moved Permanently"},
		{302, "Found"},
		{303, "See Other"},
		{304, "Not Modified"},
		{305, "Use Proxy"},
		{307, "Temporary Redirect"},
		{308, "Permanent Redirect"},
		{400, "Bad Request"},
		{401, "Unauthorized"},
		{402, "Payment Required"},
		{403, "Forbidden"},
		{404, "Not Found"},
		{405, "Method Not Allowed"},
		{406, "Not Acceptable"},
		{407, "Proxy Authentication Required"},
		{408, "Request Timeout"},
		{409, "Conflict"},
		{410, "Gone"},
		{411, "Length Required"},
		{412, "Precondition Failed"},
		{413, "Payload Too Large"},
		{414, "URI Too Long"},
		{415, "Unsupported Media Type"},
		{416, "Range Not Satisfiable"},
		{417, "Expectation Failed"},
		{421, "Misdirected Request"},
		{422, "Unprocessable Entity"},
		{423, "Locked"},
		{424, "Failed Dependency"},
		{426, "Upgrade Required"},
		{428, "Precondition Required"},
		{429, "Too Many Requests"},
		{431, "Request Header Fields Too Large"},
		{451, "Unavailable For Legal Reasons"},
		{500, "Internal Server Error"},
		{501, "Not Implemented"},
		{502, "Bad Gateway"},
		{503, "Service Unavailable"},
		{504, "Gateway Timeout"},
		{505, "HTTP Version Not Supported"},
		{506, "Variant Also Negotiates"},
		{507, "Insufficient Storage"},
		{508, "Loop Detected"},
		{510, "Not Extended"},
		{511, "Network Authentication Required"},
	};
}
}
}
