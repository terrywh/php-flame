#pragma once

namespace flame {
namespace net {
namespace fastcgi {
	enum {
		// VERSION
		PV_VERSION = 1,
		// TYPE
		PT_BEGIN_REQUEST = 1,
		PT_ABORT_REQUEST = 2,
		PT_END_REQUEST   = 3,
		PT_PARAMS        = 4,
		PT_STDIN         = 5,
		PT_STDOUT        = 6,
		PT_STDERR        = 7,
		PT_DATA          = 8,
		PT_GET_VALUES    = 9,
		PT_GET_VALUES_RESULT = 10,
		PT_UNKNOWN_TYPE  = 11,
		// FLAGS
		PF_KEEP_CONN = 0x01,
		// ROLE
		PR_RESPONDER  = 1,
		PR_AUTHORIZER = 2,
		PR_FILTER     = 3,
	};
	void init(php::extension_entry& ext);
}
}
}
