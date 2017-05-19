#include "../../vendor.h"
#include "init.h"
#include "server.h"
#include "server_request.h"
#include "server_response.h"
#include "header.h"

namespace net { namespace http {
	evbuffer*  REPLY_NOT_FOUND  = nullptr;

	void init(php::extension_entry& extension) {
		// 内置的响应
		REPLY_NOT_FOUND  = evbuffer_new();
		evbuffer_add(REPLY_NOT_FOUND, "page not found", 14);

		server::init(extension);
		server_request::init(extension);
		server_response::init(extension);
		header::init(extension);
	}
}}
