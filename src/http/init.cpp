#include "../vendor.h"
#include "init.h"
#include "header.h"
#include "server.h"
#include "server_request.h"
#include "server_response.h"
#include "client.h"
#include "client_request.h"
#include "client_response.h"

namespace http {
	evbuffer*  REPLY_NOT_FOUND  = nullptr;

	void init(php::extension_entry& extension) {
		// 内置的响应
		REPLY_NOT_FOUND  = evbuffer_new();
		evbuffer_add(REPLY_NOT_FOUND, "page not found", 14);

		header::init(extension);
		server::init(extension);
		server_request::init(extension);
		server_response::init(extension);
		client::init(extension);
		client_request::init(extension);
		client_response::init(extension);
	}

	php::string command2method(evhttp_cmd_type cmd) {
		switch(cmd) {
			case EVHTTP_REQ_GET:
				return php::string("GET", 3);
			case EVHTTP_REQ_POST:
				return php::string("POST", 4);
			case EVHTTP_REQ_HEAD:
				return php::string("HEAD", 4);
			case EVHTTP_REQ_PUT:
				return php::string("PUT", 3);
			case EVHTTP_REQ_DELETE:
				return php::string("DELETE", 6);
			case EVHTTP_REQ_OPTIONS:
				return php::string("OPTIONS", 7);
			case EVHTTP_REQ_TRACE:
				return php::string("TRACE", 5);
			case EVHTTP_REQ_CONNECT:
				return php::string("CONNECT", 7);
			case EVHTTP_REQ_PATCH:
				return php::string("PATCH", 5);
		}
	}

	evhttp_cmd_type method2command(const php::string& method) {
		if(evutil_ascii_strncasecmp(method.c_str(), "GET", 3) == 0) {
			return EVHTTP_REQ_GET;
		}else if(evutil_ascii_strncasecmp(method.c_str(), "POST", 4) == 0) {
			return EVHTTP_REQ_POST;
		}else if(evutil_ascii_strncasecmp(method.c_str(), "HEAD", 4) == 0) {
			return EVHTTP_REQ_HEAD;
		}else if(evutil_ascii_strncasecmp(method.c_str(), "PUT", 3) == 0) {
			return EVHTTP_REQ_PUT;
		}else if(evutil_ascii_strncasecmp(method.c_str(), "DELETE", 6) == 0) {
			return EVHTTP_REQ_DELETE;
		}else if(evutil_ascii_strncasecmp(method.c_str(), "OPTIONS", 7) == 0) {
			return EVHTTP_REQ_OPTIONS;
		}else if(evutil_ascii_strncasecmp(method.c_str(), "TRACE", 5) == 0) {
			return EVHTTP_REQ_TRACE;
		}else if(evutil_ascii_strncasecmp(method.c_str(), "CONNECT", 7) == 0) {
			return EVHTTP_REQ_CONNECT;
		}else if(evutil_ascii_strncasecmp(method.c_str(), "PATCH", 5) == 0) {
			return EVHTTP_REQ_PATCH;
		}else{
			throw php::exception("unsupported request method");
		}
	}
}
