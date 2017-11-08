#include "../../coroutine.h"
#include "fastcgi.h"
#include "server_connection.h"
#include "../http/server_handler.h"
#include "server_response.h"

namespace flame {
namespace net {
namespace fastcgi {

	void init(php::extension_entry& ext) {
		php::class_entry<handler_t> class_handler("flame\\net\\fastcgi\\handler");
		class_handler.add(php::property_entry("__CONNECTION_HANDLER__", (bool)true));
		class_handler.add<&handler_t::put>("put");
		class_handler.add<&handler_t::remove>("delete");
		class_handler.add<&handler_t::post>("post");
		class_handler.add<&handler_t::get>("get");
		class_handler.add<&handler_t::handle>("handle");
		class_handler.add<&handler_t::__invoke>("__invoke");
		ext.add(std::move(class_handler));

		php::class_entry<server_response> class_server_response("flame\\net\\fastcgi\\server_response");
		class_server_response.add(php::property_entry("status", 200));
		class_server_response.add(php::property_entry("header", nullptr));
		class_server_response.add(php::property_entry("header_sent", (bool)false));
		class_server_response.add(php::property_entry("ended", (bool)false));
		class_server_response.add<&server_response::__construct>("__construct", ZEND_ACC_PRIVATE);
		class_server_response.add<&server_response::write_header>("write_header");
		class_server_response.add<&server_response::write>("write");
		class_server_response.add<&server_response::end>("end");
		ext.add(std::move(class_server_response));
	}
}
}
}
