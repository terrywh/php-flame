#include "../../vendor.h"
#include "init.h"
#include "request.h"
#include "write_buffer.h"
#include "response.h"

namespace net { namespace http {
	void init(php::extension_entry& extension) {
		request::init(extension);
		response::init(extension);
	}
} }
