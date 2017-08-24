#include "../../fiber.h"
#include "client_request.h"
#include "client.h"

namespace flame {
namespace net {
namespace http {
client_request::client_request()
: curl_(nullptr)
, slist_(nullptr)
, sockfd_(-1)
, cli_(nullptr) {
	prop("header") = php::array(0);
}
php::value client_request::__construct(php::parameters& params) {
	php::string str("");
	prop("method") = str;
	if (params.length() >= 3) {
		prop("timeout") = static_cast<int>(params[2]);
	}else{
		prop("timeout") = 5;
	}
	if (params.length() >= 2) {
		prop("body") = static_cast<php::string&>(params[1]);
		prop("method") = php::string("POST");
	}else{
		prop("method") = php::string("GET");
	}
	if (params.length() >= 1) {
		prop("url") = static_cast<php::string&>(params[0]);
	}
	return this;
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
	memcpy(ptr,stream,size*nmemb);
	return size*nmemb;
}

static size_t write_callback(char* ptr, size_t size, size_t nmemb, void *userdata) {
	client_request* req = reinterpret_cast<client_request*>(userdata);
	size_t ret_len = size*nmemb > CURL_MAX_WRITE_SIZE ? CURL_MAX_WRITE_SIZE : size*nmemb;
	if (req->result_.is_empty()) {
		php::string cache(ret_len);
		req->result_ = std::move(cache);
		req->result_.length() = 0;
	} else {
		php::string cache(req->result_.length()+ret_len);
		memcpy(cache.data(), req->result_.data(), req->result_.length());
		cache.length() = req->result_.length();
		req->result_ = std::move(cache);
	}
	memcpy(req->result_.data()+req->result_.length(), ptr, ret_len);
	req->result_.length() += ret_len;
	return ret_len;
}

void client_request::build(client* cli) {
	if (curl_) {
		throw php::exception("request already used", -1);
	}
	curl_ = curl_easy_init();
	if(!curl_) {
		throw php::exception("curl_easy_init fail", -1);
	}
	curl_slist* slist = build_header();
	if(!slist) {
		curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, slist);
	}
	// 设置URL
	php::string& url = prop("url");
	if (!url.is_empty()) {
		curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
	} else {
		throw php::exception("url empty", -1);
	}
	this->cli_ = cli;
	curl_easy_setopt(curl_, CURLOPT_WRITEDATA, (void*)this);
	curl_easy_setopt(curl_, CURLOPT_PRIVATE, (void*)this);
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
			curl_easy_setopt(curl_, CURLOPT_READFUNCTION, read_callback);
		}
	}
	curl_easy_setopt(curl_, CURLOPT_TIMEOUT, static_cast<long>(prop("timeout")));
	curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2);
}

void client_request::release() {
	if (slist_) {
		curl_slist_free_all(slist_);
		slist_ = nullptr;
	}

	memset(&poll_handle_,0,sizeof(poll_handle_));
	curl_   = nullptr;
	sockfd_ = -1;
	cli_    = nullptr;
	result_ = nullptr;
}
curl_slist* client_request::build_header() {
	if (slist_) return slist_;

	php::array header = prop("header");
	if (header.length()) {
		return nullptr;
	} else {
		php::string str;
		for(auto i=header.begin();i!=header.end();++i) {
			php::string& key = i->first;
			php::string& val = i->second;
			php::string  str(key.length() + val.length() + 3);
			sprintf(str.data(), "%.*s: %.*s", key.length(), key.data(), val.length(), val.data());
			slist_ = curl_slist_append(slist_, str.c_str());
		}
		return slist_;
	}
}

}
}
}
