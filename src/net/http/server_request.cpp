#include "../../vendor.h"
#include "request.h"

namespace net { namespace http {
	void server_request::init(php::extension_entry& extension) {
		php::class_entry<request> class_server_request("flame\\http\\server_request");
		extension.add(std::move(class_server_request));
	}
}}
