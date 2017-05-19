#pragma once

namespace net { namespace http {
	void init(php::extension_entry& extension);
	extern evbuffer*  REPLY_NOT_FOUND;
	php::string command2method(evhttp_cmd_type cmd);
	evhttp_cmd_type method2command(const php::string& method);
}}
