#include "../../vendor.h"
#include "init.h"
#include "server_request.h"
#include "write_buffer.h"
#include "server_response.h"
#include "agent.h"

namespace net { namespace http {
	void init(php::extension_entry& extension) {
		server_request::init(extension);
		server_response::init(extension);
		agent::init(extension);
	}
} }
