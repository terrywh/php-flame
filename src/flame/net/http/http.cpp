#include "http.h"
#include "client.h"
#include "server.h"

namespace flame {
namespace net {
namespace http {
	void init(php::extension_entry& ext) {
		// class_client
		// ------------------------------------
		if(curl_global_init(CURL_GLOBAL_ALL)) {
			php::exception("curl_glbal_init fail", -1);
		}

		ext.add<http::get>("flame\\net\\http\\get");
		ext.add<http::post>("flame\\net\\http\\post");
		ext.add<http::put>("flame\\net\\http\\put");

		php::class_entry<http::request> class_request("flame\\net\\http\\request");
		class_request.add(php::property_entry("url", ""));
		class_request.add(php::property_entry("method", ""));
		class_request.add(php::property_entry("timeout", 10));
		class_request.add(php::property_entry("header", nullptr));
		class_request.add(php::property_entry("body", nullptr));
		class_request.add<&http::request::__construct>("__construct");
		class_request.add<&http::request::__destruct>("__destruct");
		ext.add(std::move(class_request));

		php::class_entry<http::client> class_client("flame\\net\\http\\client");
		class_client.add<&http::client::exec>("exec");
		class_client.add<&http::client::debug>("debug");
		ext.add(std::move(class_client));
		// class_server
		// -----------------------------------
		php::class_entry<server> class_server("flame\\net\\http\\server");
		class_server.add(php::property_entry("local_address", std::string("")));
		class_server.add(php::property_entry("local_port", 0));
		class_server.add<&server::bind>("bind");
		class_server.add<&server::handle>("handle");
		class_server.add<&server::run>("run");
		class_server.add<&server::close>("close");
		class_server.add<&server::__destruct>("__destruct");
		ext.add(std::move(class_server));
	}
}
}
}
