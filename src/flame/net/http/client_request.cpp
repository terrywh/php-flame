#include "../../flame.h"
#include "../../coroutine.h"
#include "client_request.h"
#include "client.h"
#include "client_response.h"

namespace flame {
namespace net {
namespace http {
client_request::client_request()
: curl_(nullptr)
, curl_header(nullptr)
, curl_fd(-1)
, cli_(nullptr) {
	poll_ = (uv_poll_t*)malloc(sizeof(uv_poll_t));
	poll_->data = this;
	curl_ = curl_easy_init();
	curl_easy_setopt(curl_, CURLOPT_PRIVATE, this);
	res_obj = php::object::create<client_response>();
	res_    = res_obj.native<client_response>();
	res_->init();
}
php::value client_request::__construct(php::parameters& params) {
	if (params.length() >= 3) {
		prop("timeout") = static_cast<int>(params[2]);
	}else{
		prop("timeout") = 2500;
	}
	if (params.length() >= 2) {
		prop("body")   = params[1];
		prop("method") = php::string("POST");
	}else{
		prop("method") = php::string("GET");
	}
	if (params.length() >= 1) {
		prop("url") = static_cast<php::string&>(params[0]);
	}
	prop("header") = php::array(0);
	prop("cookie") = php::array(0);
	return this;
}
php::value client_request::ssl(php::parameters& params) {
	php::array& opt = params[0];
	php::value* vvf = opt.find("verify");
	if(vvf != nullptr) {
		php::string& val = *vvf;
		if(std::strncmp(val.c_str(), "host", 4) == 4) {
			curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 1);
			curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0);
		}else if(std::strncmp(val.c_str(), "peer", 4) == 0) {
			curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0);
			curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1);
		}else if(std::strncmp(val.c_str(), "both", 4) == 0) {
			curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 1);
			curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1);
		}else{ // "none"
			curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0);
			curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0);
		}
	}
	php::value* crt = opt.find("cert");
	CURLcode r = CURLE_UNKNOWN_OPTION;
	if(crt != nullptr) {
		php::string& val = *crt;
		if(std::strncmp(val.c_str() + val.length() - 4, ".pem", 4) == 0) {
			curl_easy_setopt(curl_, CURLOPT_SSLCERTTYPE, "PEM");
		}else if(std::strncmp(val.c_str() + val.length() - 4, ".der", 4) == 0) {
			curl_easy_setopt(curl_, CURLOPT_SSLCERTTYPE, "DER");
		}
		r = curl_easy_setopt(curl_, CURLOPT_SSLCERT, val.c_str());
	}
	if(r != CURLE_OK) {
		throw php::exception("unsupported certificate");
	}
	php::value* key = opt.find("key");
	if(key != nullptr) {
		php::string& val = *crt;
		if(std::strncmp(val.c_str() + val.length() - 4, ".pem", 4) == 0) {
			curl_easy_setopt(curl_, CURLOPT_SSLCERTTYPE, "PEM");
		}else if(std::strncmp(val.c_str() + val.length() - 4, ".der", 4) == 0) {
			curl_easy_setopt(curl_, CURLOPT_SSLCERTTYPE, "DER");
		}else if(std::strncmp(val.c_str() + val.length() - 4, ".eng", 4) == 0) {
			curl_easy_setopt(curl_, CURLOPT_SSLCERTTYPE, "ENG");
		}
		r = curl_easy_setopt(curl_, CURLOPT_SSLKEY, val.c_str());
	}
	if(r != CURLE_OK) {
		throw php::exception("unsupported private key");
	}
	php::value* pass = opt.find("pass");
	if(pass != nullptr) {
		php::string& val = *pass;
		curl_easy_setopt(curl_, CURLOPT_KEYPASSWD, val.c_str());
	}
	return nullptr;
}

size_t client_request::read_cb(void *ptr, size_t size, size_t nmemb, void *stream) {
	memcpy(ptr, stream, size*nmemb);
	return size*nmemb;
}
size_t client_request::head_cb(char* ptr, size_t size, size_t nitems, void* userdata) {
	client_response* req = reinterpret_cast<client_response*>(userdata);
	size_t           len = size*nitems;
	req->head_cb(ptr, len);
	return len;
}
size_t client_request::body_cb(char* ptr, size_t size, size_t nmemb, void *userdata) {
	client_response* req = reinterpret_cast<client_response*>(userdata);
	size_t           len = size*nmemb;
	req->body_cb(ptr, len);
	return len;
}
bool client_request::done_cb(CURLMsg* message) {
	switch(message->msg) {
	case CURLMSG_DONE:
		if (message->data.result != CURLE_OK) {
			co_->fail(curl_easy_strerror(message->data.result), message->data.result);
		} else {
			php::object      obj = php::object::create<client_response>();
			client_response* cpp = obj.native<client_response>();
			cpp->done_cb(curl_);
			co_->next(std::move(obj));
		}
		return true;
	default:
		assert(0);
		return false;
	}
}

void client_request::build(client* cli) {
	curl_slist* header = build_header();
	if(!header) {
		curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, header);
	}
	this->cli_ = cli;
	// 设置URL
	php::string& url = prop("url");
	if (!url.is_empty()) {
		curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
	} else {
		throw php::exception("client_request build failed: empty url", -1);
	}
	php::buffer cookie;
	php::array& cookie_all = prop("cookie");
	for(auto i=cookie_all.begin(); i!= cookie_all.end(); ++i) {
		php::string key = i->first.to_string();
		php::string val = i->second.to_string();
		std::memcpy(cookie.put(key.length()), key.c_str(), key.length());
		cookie.add('=');
		std::memcpy(cookie.put(val.length()), val.c_str(), val.length());
		cookie.add(';');
		cookie.add(' ');
	}
	php::string cookie_str = std::move(cookie); // 这里会添加 '\0' 结束符
	curl_easy_setopt(curl_, CURLOPT_READDATA, cookie_str.c_str());
	std::string method = prop("method");
	php::value   vbody = prop("body");
	php::string  xbody;
	if (vbody.is_array()) {
		xbody = php::build_query(vbody);
		prop("body") = xbody;
	}else if (vbody.is_string()) {
		xbody = vbody;
	}else if (!vbody.is_null()) {
		xbody = vbody.to_string();
	}
	if(method.compare("POST") == 0) {
		curl_easy_setopt(curl_, CURLOPT_POST, 1L);
		if(xbody.length() > 0) {
			curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, xbody.c_str());
		}
	}else if(method.compare("PUT") == 0) {
		curl_easy_setopt(curl_, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl_, CURLOPT_PUT, 1L);
		if(xbody.length() > 0) {
			curl_easy_setopt(curl_, CURLOPT_READDATA, xbody.c_str());
			curl_easy_setopt(curl_, CURLOPT_INFILESIZE, xbody.length());
			curl_easy_setopt(curl_, CURLOPT_READFUNCTION, read_cb);
		}
	}
	curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, static_cast<long>(prop("timeout")));
	// curl_easy_setopt(curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2);
	curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, body_cb);
	curl_easy_setopt(curl_, CURLOPT_WRITEDATA, res_);
	// 数据接收头部，但非 curl_easy_setopt(curl_, CURLOPT_HEADER, 1L);
	curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, head_cb);
	curl_easy_setopt(curl_, CURLOPT_HEADERDATA, res_);
	if (cli->debug_) {
		curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1L);
	}
}

void client_request::close() {
	if (curl_header) {
		curl_slist_free_all(curl_header);
		curl_header = nullptr;
		curl_easy_cleanup(curl_);
		curl_   = nullptr;
		uv_close((uv_handle_t*)poll_, free_handle_cb);
		poll_   = nullptr;
		curl_fd = -1;
		cli_    = nullptr;
	}
}
curl_slist* client_request::build_header() {
	if (curl_header) return curl_header;

	php::array header = prop("header");
	if (header.length()) {
		return nullptr;
	} else {
		bool expect = false;
		for(auto i=header.begin();i!=header.end();++i) {
			php::string& key = i->first;
			php::string& val = i->second;
			if(std::strncmp(key.c_str(), "Expect", 6) == 0 || std::strncmp(key.c_str(), "expect", 6) == 0) {
				expect = true;
			}
			php::string  str(key.length() + val.length() + 3);
			sprintf(str.data(), "%.*s: %.*s", key.length(), key.data(), val.length(), val.data());
			curl_header = curl_slist_append(curl_header, str.c_str());
		}
		if(!expect) {
			curl_header = curl_slist_append(curl_header, "Expect: ");
		}
		return curl_header;
	}
}

}
}
}
