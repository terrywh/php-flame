#include "../../vendor.h"
#include "init.h"
#include "request.h"
#include "response.h"

namespace net { namespace http {
	void init(php::extension_entry& extension) {
        request::init(extension);
        response::init(extension);
		/*php::class_entry<request> mill_http_request("mill\\http\\request");*/
		//mill_http_request.add<request::parse>("parse");
		//mill_http_request.add<&request::__construct>("__construct");
		//mill_http_request.add(php::property_entry("version", "HTTP/1.1"));
		//mill_http_request.add(php::property_entry("method", "GET"));
		//mill_http_request.add(php::property_entry("uri", "/"));
		//mill_http_request.add(php::property_entry("body", nullptr));
		//mill_http_request.add(php::property_entry("header", nullptr));
		//mill_http_request.add(php::property_entry("query", nullptr));
		//mill_http_request.add(php::property_entry("cookie", nullptr));
		/*extension.add(std::move(mill_http_request));*/
	}
} }
