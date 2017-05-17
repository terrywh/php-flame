#include "../../vendor.h"
#include "init.h"
#include "server.h"
#include "request.h"
#include "response.h"

namespace net { namespace http {
	evbuffer*  REPLY_NOT_FOUND  = nullptr;

	void init(php::extension_entry& extension) {
		// 内置的响应
		REPLY_NOT_FOUND  = evbuffer_new();
		evbuffer_add(REPLY_NOT_FOUND, "page not found", 14);

		server::init(extension);
		request::init(extension);
		response::init(extension);
	}
}}
