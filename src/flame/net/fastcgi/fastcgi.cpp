#include "fastcgi.h"
#include "server.h"
#include "server_response.h"

namespace flame {
namespace net {
namespace fastcgi {
	void init(php::extension_entry& ext) {
		php::class_entry<server> class_server("flame\\net\\fastcgi\\server");
		class_server.add(php::property_entry("local_address", ""));
		class_server.add<&server::handle>("handle");
		class_server.add<&server::bind>("bind");
		class_server.add<&server::run>("run");
		class_server.add<&server::close>("close");
		ext.add(std::move(class_server));

		php::class_entry<server_response> class_server_response("flame\\net\\fastcgi\\server_response");
		class_server_response.add(php::property_entry("status", 200));
		class_server_response.add(php::property_entry("header", nullptr));
		class_server_response.add(php::property_entry("header_sent", false));
		class_server_response.add(php::property_entry("ended", false));
		class_server_response.add<&server_response::write_header>("write_header");
		class_server_response.add<&server_response::write>("write");
		class_server_response.add<&server_response::end>("end");
		ext.add(std::move(class_server_response));
	}
}
}
}
