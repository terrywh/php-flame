#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "http.h"
#include "client.h"
#include "client_request.h"
#include "client_response.h"
#include "server_connection_base.h"
#include "server_connection.h"
#include "server_request.h"
#include "server_response_base.h"
#include "server_response.h"
#include "server_handler.h"

namespace flame {
namespace net {
namespace http {
	typedef server_handler<server_connection> http_server_handler;

	void init(php::extension_entry& ext) {
		curl_global_init(CURL_GLOBAL_DEFAULT);
		ext.on_module_startup([] (php::extension_entry& ext) -> bool {
			default_client = new client();
			default_client->default_options();
			return true;
		});
		ext.on_module_shutdown([] (php::extension_entry& ext) -> bool {
			delete default_client;
			curl_global_cleanup();
			return true;
		});
		ext.add(php::constant_entry("flame\\net\\http\\HTTP_VERSION_AUTO", CURL_HTTP_VERSION_NONE));
		ext.add(php::constant_entry("flame\\db\\mysql\\HTTP_VERSION_1_0", CURL_HTTP_VERSION_1_0));
		ext.add(php::constant_entry("flame\\db\\mysql\\HTTP_VERSION_1_1", CURL_HTTP_VERSION_1_1));
		ext.add(php::constant_entry("flame\\db\\mysql\\HTTP_VERSION_2_0", CURL_HTTP_VERSION_2_0));
		ext.add(php::constant_entry("flame\\db\\mysql\\HTTP_VERSION_2TLS", CURL_HTTP_VERSION_2TLS));
		ext.add(php::constant_entry("flame\\db\\mysql\\HTTP_VERSION_2_PRIOR_KNOWLEDGE", CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE));
		// shortcut for class_client
		ext.add<::flame::net::http::get>("flame\\net\\http\\get");
		ext.add<::flame::net::http::post>("flame\\net\\http\\post");
		ext.add<::flame::net::http::put>("flame\\net\\http\\put");
		ext.add<::flame::net::http::remove>("flame\\net\\http\\delete");
		ext.add<::flame::net::http::exec>("flame\\net\\http\\exec");
		// class client_request
		php::class_entry<client_request> class_client_request("flame\\net\\http\\client_request");
		class_client_request.add(php::property_entry("method", std::string("")));
		class_client_request.add(php::property_entry("url", std::string("")));
		class_client_request.add(php::property_entry("header", nullptr));
		class_client_request.add(php::property_entry("cookie", nullptr));
		class_client_request.add(php::property_entry("body", nullptr));
		class_client_request.add(php::property_entry("timeout", 3000));
		class_client_request.add<&client_request::__construct>("__construct");
		class_client_request.add<&client_request::ssl>("ssl");
		ext.add(std::move(class_client_request));
		// class client_response
		php::class_entry<client_response> class_client_response("flame\\net\\http\\client_response");
		class_client_response.add(php::property_entry("status", 0));
		class_client_response.add(php::property_entry("header", nullptr));
		class_client_response.add(php::property_entry("cookie", nullptr));
		class_client_response.add(php::property_entry("body", nullptr));
		class_client_response.add<&client_response::__construct>("__construct", ZEND_ACC_PRIVATE); // 私有构造
		class_client_response.add<&client_response::to_string>("__toString");
		ext.add(std::move(class_client_response));
		// class client
		php::class_entry<client> class_client("flame\\net\\http\\client");
		class_client.add<&client::__construct>("__construct");
		class_client.add<&client::exec1>("exec");
		ext.add(std::move(class_client));
		// class handler
		php::class_entry<http_server_handler> class_handler("flame\\net\\http\\handler");
		class_handler.add(php::property_entry("__CONNECTION_HANDLER__", (zend_bool)true));
		class_handler.add<&http_server_handler::put>("put");
		class_handler.add<&http_server_handler::remove>("delete");
		class_handler.add<&http_server_handler::post>("post");
		class_handler.add<&http_server_handler::get>("get");
		class_handler.add<&http_server_handler::handle>("handle");
		class_handler.add<&http_server_handler::before>("before");
		class_handler.add<&http_server_handler::after>("after");
		class_handler.add<&http_server_handler::__invoke>("__invoke");
		ext.add(std::move(class_handler));
		// class server_request
		php::class_entry<server_request> class_server_request("flame\\net\\http\\server_request");
		class_server_request.add(php::property_entry("method", std::string("")));
		class_server_request.add(php::property_entry("uri", std::string("")));
		class_server_request.add(php::property_entry("query", nullptr));
		class_server_request.add(php::property_entry("header", nullptr));
		class_server_request.add(php::property_entry("cookie", nullptr));
		class_server_request.add(php::property_entry("body", nullptr));
		class_server_request.add(php::property_entry("rawBody", std::string("")));
		class_server_request.add(php::property_entry("data", nullptr));
		class_server_request.add<&server_request::__construct>("__construct", ZEND_ACC_PRIVATE); // 私有构造
		ext.add(std::move(class_server_request));
		// class server_request
		php::class_entry<server_response> class_server_response("flame\\net\\http\\server_response");
		class_server_response.add(php::property_entry("status", 200));
		class_server_response.add(php::property_entry("header", nullptr));
		class_server_response.add(php::property_entry("data", nullptr));
		class_server_response.add<&server_response::__construct>("__construct", ZEND_ACC_PRIVATE); // 私有构造
		class_server_response.add<&server_response::set_cookie>("set_cookie");
		class_server_response.add<&server_response::write_header>("write_header");
		class_server_response.add<&server_response::write>("write");
		class_server_response.add<&server_response::end>("end");
		ext.add(std::move(class_server_response));
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
