#include "../../coroutine.h"
#include "../http/server_connection_base.h"
#include "../http/server_handler.h"
#include "../http/server_response_base.h"
#include "fastcgi.h"
#include "server_connection.h"
#include "server_response.h"

namespace flame {
namespace net {
namespace fastcgi {
	typedef http::server_handler<server_connection> handler_t;

	void init(php::extension_entry& ext) {
		php::class_entry<handler_t> class_handler("flame\\net\\fastcgi\\handler");
		class_handler.add(php::property_entry("__CONNECTION_HANDLER__", (zend_bool)true));
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
		class_server_response.add<&server_response::__construct>("__construct", ZEND_ACC_PRIVATE); // 私有构造
		class_server_response.add<&server_response::set_cookie>("set_cookie");
		class_server_response.add<&server_response::write_header>("write_header");
		class_server_response.add<&server_response::write>("write");
		class_server_response.add<&server_response::end>("end");
		ext.add(std::move(class_server_response));
	}
}
}
}
