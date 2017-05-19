#pragma once

namespace net { namespace http {
	void init(php::extension_entry& extension);
	extern evbuffer*  REPLY_NOT_FOUND;
}}
