#include "fastcgi.h"
#include "server.h"
#include "connection.h"

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
	}
}
}
}
