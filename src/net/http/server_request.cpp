#include "../../vendor.h"
#include "server_request.h"
#include "header.h"
#include "init.h"

namespace net { namespace http {
	void server_request::init(php::extension_entry& extension) {
		php::class_entry<server_request> class_server_request("flame\\http\\server_request");
		class_server_request.add(php::property_entry("method",      ""));
		class_server_request.add(php::property_entry("path",        ""));
		class_server_request.add(php::property_entry("get",    nullptr));
		class_server_request.add(php::property_entry("header", nullptr));
		class_server_request.add(php::property_entry("cookie", nullptr));
		class_server_request.add(php::property_entry("body",        ""));
		class_server_request.add(php::property_entry("post",   nullptr));
		extension.add(std::move(class_server_request));
	}

	void server_request::init(evhttp_request* evreq, server* svr) {
		req_ = evreq;
		// METHOD
		prop("method", 6) = command2method(evhttp_request_get_command(req_));
		// PATH and GET QUERY
		char* uri   = const_cast<char*>(evhttp_request_get_uri(req_));
		char* query = std::strstr(uri, "?");
		if(query == nullptr) {
			prop("path", 4) = php::value(uri, std::strlen(uri));
		}else{
			prop("path", 4) = php::value(uri, query - uri);
			prop("get",  3)  = php::parse_str('&', query+1, std::strlen(query+1));
		}
		// HEADER
		php::object hdr_= php::object::create<header>();
		header*     hdr = hdr_.native<header>();
		hdr->init(evhttp_request_get_input_headers(req_));
		prop("header", 6) = std::move(hdr_);
		// COOKIE
		const char* cookie = evhttp_find_header(hdr->queue_, "Cookie");
		if(cookie != nullptr) {
			// !!! 这里 cookie 同 key 覆盖，相当于后者生效（与 PHP 的默认行为不符）
			prop("cookie", 6) = php::parse_str(';', cookie, std::strlen(cookie));
		}
		// BODY
		evbuffer* body = evhttp_request_get_input_buffer(req_);
		php::string post(evbuffer_get_length(body));
		// TODO 这个内存复制代价有可能会比较大，是否有希望能变为 流 的方式来返回？
		evbuffer_remove(body, post.data(), post.length());
		prop("body", 4) = post;
		// POST
		const char* ctype = evhttp_find_header(hdr->queue_, "Content-Type");
		if(ctype != nullptr && std::strncmp("application/x-www-form-urlencoded", ctype, 33) == 0) {
			prop("post", 4) = php::parse_str('&', post.data(), post.length());
		}
	}
}}
