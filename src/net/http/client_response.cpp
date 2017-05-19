#include "../../vendor.h"
#include "header.h"
#include "client_response.h"


namespace net { namespace http {
	void client_response::init(php::extension_entry& extension) {
		php::class_entry<client_response> class_client_response("flame\\net\\http\\client_response");
		class_client_response.add<&client_response::__toString>("__toString");
		class_client_response.add(php::property_entry("header", nullptr));
		class_client_response.add(php::property_entry("body", ""));
		extension.add(std::move(class_client_response));
	}
	void client_response::init(evhttp_request* req) {
		req_ = req;
		// HEADER
		php::object hdr_= php::object::create<header>();
		header* hdr = hdr_.native<header>();
		hdr->init(evhttp_request_get_output_headers(req_));
		prop("header") = std::move(hdr_);
		// BODY
		evbuffer* buffer = evhttp_request_get_input_buffer(req_);
		std::size_t size = evbuffer_get_length(buffer);
		php::string body(size);
		evbuffer_remove(buffer, body.data(), size);
		prop("body") = std::move(body);
	}

	client_response::client_response()
	: req_(nullptr)
	, keepalive_(true) {}

	client_response::~client_response() {

	}

	php::value client_response::__toString(php::parameters& params) {
		return prop("body");
	}

}}
