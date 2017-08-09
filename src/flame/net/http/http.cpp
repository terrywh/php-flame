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
		class_client_request.add<&client_request::__destruct>("__destruct");
		ext.add(std::move(class_client_request));
		// class client
		php::class_entry<client> class_client("flame\\net\\http\\client");
		class_client.add<&client::exec>("exec");
		class_client.add<&client::debug>("debug");
		ext.add(std::move(class_client));
		// class server_request
		php::class_entry<server_request> class_server_request("flame\\net\\http\\server_request");
		class_server_request.add(php::property_entry("headers", nullptr));
		class_server_request.add(php::property_entry("query", nullptr));
		class_server_request.add(php::property_entry("body", nullptr));
	}
}
}
}
