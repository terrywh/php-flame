#include "client.h"

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);
namespace flame {
namespace net {
namespace http {

size_t write_callback(char* ptr, size_t size, size_t nmemb, void *userdata) {
	request* req = (request*)userdata;
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


static void check_multi_info(client* cli) {
	int pending;
	CURLMsg *message;
	while((message = curl_multi_info_read(cli->get_curl_handle(), &pending))) {
		request* req;
		CURL* easy_handle = message->easy_handle;
		curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &req);
		req->cb_(message);
		curl_multi_remove_handle(cli->get_curl_handle(), easy_handle);
		curl_easy_cleanup(easy_handle);
	}
}

void client::curl_perform(uv_poll_t *req, int status, int events) {
	int flags = 0;
	int running_handles;
	if(events & UV_READABLE)
		flags |= CURL_CSELECT_IN;
	if(events & UV_WRITABLE)
		flags |= CURL_CSELECT_OUT;
	curl_socket_t fd = ((request*)req->data)->sockfd_;
	client* cli = ((request*)req->data)->cli_;
	curl_multi_socket_action(cli->get_curl_handle(), fd, flags, &running_handles);
	check_multi_info(cli);
}


static void on_timeout(uv_timer_t *req) {
	int running_handles;
	client* cli = (client*)(req->data);
	curl_multi_socket_action(cli->get_curl_handle(), CURL_SOCKET_TIMEOUT, 0, &running_handles);
	check_multi_info(cli);
}

int client::start_timeout(CURLM* multi, long timeout_ms, void* userp) {
	if(userp) {
		client* cli = (client*)userp;
		if(timeout_ms < 0) {
			uv_timer_stop(&cli->timeout_);
		}
		else {
			if(timeout_ms == 0)
				timeout_ms = 1;
			cli->timeout_.data = cli;
			uv_timer_start(&cli->timeout_, on_timeout, timeout_ms, 0);
		}
	}
	return 0;
}


static void curl_close_cb(uv_handle_t *handle) {
	//request* req =  (request*)handle->data;
	//req->release();
}

php::value request::__construct(php::parameters& params) {
	php::string str("");
	prop("method") = str; 
	int arg_len = params.length();
	if (arg_len > 0) {
		if (arg_len == 1) {
			php::value& value = params[0];
			if (value.is_string()) {
				php::string& url = value;
				prop("url") = url;
			} else if (value.is_array()) {
				php::array& arr = value;
				prop("url") = arr["url"];
				prop("method") = arr["method"];
				php::value* vt = arr.find("timeout");
				if (vt != nullptr) {
					prop("timeout") = arr["timeout"];
				}
				php::value* vh = arr.find("header");
				if (vh != nullptr) {
					prop("header") = arr["header"];
				}
				php::value* vbody = arr.find("body");
				if (vbody != nullptr) {
					prop("body") = arr["body"];
				}
			}
		} else if( arg_len == 3) {
			php::string& method = params[0];
			prop("method") = method;
			php::string& url = params[1];
			prop("url") = url;
			php::array& body = params[2];
			prop("body") = body;
		}
	}
	return this;
}

void request::parse(client* cli) {
	if (curl_) {
		throw php::exception("request already used", -1);
	}
	curl_ = curl_easy_init();
	if(!curl_) {
		throw php::exception("curl_easy_init fail", -1);
	}
	curl_slist* slist = get_header();
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
	php::string str = get_method();
	if(str.is_empty() == false) {
		std::string method(str.c_str());
		if(method.compare("POST") == 0) {
			php::value vbody = prop("body");
			if (!vbody.is_null()) {
				php::array& body = vbody;
				php::string postfield = build_str(body);
				if (!postfield.is_null()) {
					curl_easy_setopt(curl_, CURLOPT_POST, 1L);
					curl_easy_setopt(curl_, CURLOPT_COPYPOSTFIELDS, postfield.c_str());
				}
			}
		}else if(method.compare("PUT") == 0) {
			curl_easy_setopt(curl_, CURLOPT_UPLOAD, 1L);
			curl_easy_setopt(curl_, CURLOPT_PUT, 1L);
			//curl_easy_setopt(curl, CURLOPT_READDATA, postfield.c_str());
		}else if(method.compare("GET") == 0) {
		}
	}
	curl_easy_setopt(curl_, CURLOPT_TIMEOUT, (long)get_timeout());
	curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
}

CURLM* client::get_curl_handle() {
	if (!curlm_handle_) {
		curlm_handle_ = curl_multi_init();
		uv_timer_init(flame::loop, &timeout_);
		curl_multi_setopt(curlm_handle_, CURLMOPT_SOCKETFUNCTION, handle_socket);
		curl_multi_setopt(curlm_handle_, CURLMOPT_TIMERFUNCTION, start_timeout);
		curl_multi_setopt(curlm_handle_, CURLMOPT_TIMERDATA, (void*)this);
	}
	return curlm_handle_;

}

int client::handle_socket(CURL* easy, curl_socket_t s, int action, void *userp, void *socketp) {
	request* req;
	int events = 0;
	
	switch(action) {
	case CURL_POLL_IN:
	case CURL_POLL_OUT:
	case CURL_POLL_INOUT:
	{
		curl_easy_getinfo(easy, CURLINFO_PRIVATE, &req);
		if(action != CURL_POLL_IN)
			events |= UV_WRITABLE;
		if(action != CURL_POLL_OUT)
			events |= UV_READABLE;
		if(socketp) {
			req = (request*)socketp;
		}
		else {
			req->sockfd_ = s;
			uv_poll_init_socket(flame::loop, &req->poll_handle_, s);
			req->poll_handle_.data = req;
			curl_multi_assign(req->cli_->get_curl_handle(), s, (void*)req);
		}
		uv_poll_start(&req->poll_handle_, events, client::curl_perform);
	}
	break;
	case CURL_POLL_REMOVE:
		curl_easy_getinfo(easy, CURLINFO_PRIVATE, &req);
		if(socketp) {
			uv_poll_t* p_poll_handle = &((request*)socketp)->poll_handle_;
			uv_poll_stop(p_poll_handle);
			uv_close((uv_handle_t*)p_poll_handle, curl_close_cb);
			curl_multi_assign(req->cli_->get_curl_handle(), s, nullptr);
		}
		break;
	default:
		//不可能
		break;
	}
	return 0;
};

php::value client::exec(php::object& req) {
	request* r = req.native<request>();
	r->parse(this);
	CURL* curl = r->curl_;
	if(!curl) {
		throw php::exception("curl empty", -1);
	}
	if (debug_) {
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	}
	auto fiber = flame::this_fiber();
	r->cb_ = [req, r, fiber](CURLMsg *message) {
		switch(message->msg) {
			case CURLMSG_DONE: {
				if (message->data.result != CURLE_OK) {
					php::string str_ret(curl_easy_strerror(message->data.result));
					fiber->next(std::move(str_ret));
				} else {
					fiber->next(std::move(r->result_));
				}
			}
			break;
			default: {
				fiber->next(php::string("curlmsg return not zero"));
			}
		}
	};
	curl_multi_add_handle(get_curl_handle(), curl);
	return flame::async;
}

php::value client::debug(php::parameters& params) {
	debug_ = params[0];
	return params[0];
}

php::value client::exec(php::parameters& params) {
	php::object& obj = params[0];
	if(!obj.is_instance_of<request>()) {
		php::exception("param need request", -1);
	}
	return exec(obj);
}


php::value get(php::parameters& params) {
	static client cli;
	//static php::object obj_cli = php::object::create<client>();
	//client* cli = obj_cli.native<client>();
	php::object obj_req = php::object::create<request>();
	request* req = obj_req.native<request>();
	php::string& url = params[0];
	req->prop("header") = php::array();
	req->prop("url") = url;
	req->prop("method") = php::string("GET");
	return cli.exec(obj_req);
}

php::value post(php::parameters& params) {
	static client cli;
	//static php::object obj_cli = php::object::create<client>();
	//client* cli = obj_cli.native<client>();
	php::object obj_req = php::object::create<request>();
	request* req = obj_req.native<request>();
	php::string& url = params[0];
	req->prop("url") = url;
	req->prop("method") = php::string("POST");
	req->prop("header") = php::array();
	php::array& arr = params[1];
	req->prop("body") = arr;
	return cli.exec(obj_req);
}

php::value put(php::parameters& params) {
	static client cli;
	php::object obj_req = php::object::create<request>();
	request* req = obj_req.native<request>();
	php::string url = params[0];
	req->prop("url") = url;
	req->prop("method") = php::string("PUT");
	req->prop("header") = php::array();
	php::array& arr = params[1];
	req->prop("body") = arr;
	return cli.exec(obj_req);
}


}
}
}

