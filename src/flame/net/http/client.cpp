#include "../../coroutine.h"
#include "client.h"
#include "client_request.h"

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);
namespace flame {
namespace net {
namespace http {

php::value client::__construct(php::parameters& params) {
	if(params.length() == 0 || !params[0].is_array()) {
		return nullptr;
	}
	php::array& options = params[0];
	php::value* debug = options.find("debug");
	if(debug != nullptr) {
		debug_ = debug->is_true();
	}
	// TODO 接收选项用于控制连接数量

	return nullptr;
}

void client::curl_multi_info_check(client* self) {
	int pending;
	CURLMsg *message;
	while((message = curl_multi_info_read(self->curlm_, &pending))) {
		client_request* req;
		CURL* easy_handle = message->easy_handle;
		curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &req);
		req->done_cb(message);
		curl_multi_remove_handle(self->curlm_, easy_handle);
		req->close();
		req->ref_ = nullptr;
	}
}

void client::curl_multi_socket_poll(uv_poll_t *poll, int status, int events) {
	int flags = 0;
	int running_handles;
	if(events & UV_READABLE)
		flags |= CURL_CSELECT_IN;
	if(events & UV_WRITABLE)
		flags |= CURL_CSELECT_OUT;
	client_request* req = reinterpret_cast<client_request*>(poll->data);
	curl_multi_socket_action(req->cli_->curlm_, req->curl_fd, flags, &running_handles);
	curl_multi_info_check(req->cli_);
}

void client::curl_multi_timer_cb(uv_timer_t *req) {
	int running_handles;
	client* self = reinterpret_cast<client*>(req->data);
	curl_multi_socket_action(self->curlm_, CURL_SOCKET_TIMEOUT, 0, &running_handles);
	curl_multi_info_check(self);
}

int client::curl_multi_timer_handle(CURLM* multi, long timeout_ms, void* userp) {
	if(userp) {
		client* self = reinterpret_cast<client*>(userp);
		if(timeout_ms < 0) {
			uv_timer_stop(&self->timer_);
		} else {
			if(timeout_ms == 0) timeout_ms = 1;
			uv_timer_start(&self->timer_, curl_multi_timer_cb, timeout_ms, 0);
		}
	}
	return 0;
}

int client::curl_multi_socket_handle(CURL* easy, curl_socket_t s, int action, void *userp, void *socketp) {
	client_request* req;
	int events = 0;
	curl_easy_getinfo(easy, CURLINFO_PRIVATE, &req);
	switch(action) {
	case CURL_POLL_IN:
	case CURL_POLL_OUT:
	case CURL_POLL_INOUT:
	{
		if(action != CURL_POLL_IN) events |= UV_WRITABLE;
		if(action != CURL_POLL_OUT) events |= UV_READABLE;
		req->setfd(s);
		uv_poll_start(req->poll_, events, client::curl_multi_socket_poll);
	}
	break;
	case CURL_POLL_REMOVE:
		req->curl_fd = -1;
		uv_poll_stop(req->poll_);
	break;
	default:
		assert(0);
	}
	return 0;
};

client::client()
: debug_(0) {
	curlm_ = curl_multi_init();
	uv_timer_init(flame::loop, &timer_);
	timer_.data = this;
	curl_multi_setopt(curlm_, CURLMOPT_SOCKETFUNCTION, curl_multi_socket_handle);
	curl_multi_setopt(curlm_, CURLMOPT_SOCKETDATA, this);
	curl_multi_setopt(curlm_, CURLMOPT_TIMERFUNCTION, curl_multi_timer_handle);
	curl_multi_setopt(curlm_, CURLMOPT_TIMERDATA, this);
}

php::value client::exec2(php::object& obj) {
	if(!obj.is_instance_of<client_request>()) {
		throw php::exception("only instanceof 'class client_request' can be executed");
	}
	client_request* cpp = obj.native<client_request>();
	if(cpp->curl_ == nullptr) {
		throw php::exception("request object can NOT be reused");
	}
	cpp->co_  = coroutine::current;
	cpp->ref_ = obj; // 异步运行过程，保留引用
	cpp->build(this);
	curl_multi_add_handle(curlm_, cpp->curl_);
	return flame::async();
}

php::value client::debug(php::parameters& params) {
	debug_ = params[0];
	return params[0];
}

php::value client::exec1(php::parameters& params) {
	php::object& obj = params[0];
	if(!obj.is_instance_of<client_request>()) {
		php::exception("object of type 'client_request' is required");
	}
	return exec2(obj);
}

void client::destroy() {
	if (curlm_) {
		uv_timer_stop(&timer_);
		curl_multi_cleanup(curlm_);
		curlm_ = nullptr;
	}
}

client* default_client = nullptr;

php::value get(php::parameters& params) {
	php::object     obj  = php::object::create<client_request>();
	client_request* req  = obj.native<client_request>();
	req->prop("method")  = php::string("GET");
	req->prop("url")     = params[0];
	req->prop("header")  = php::array(0);
	req->prop("cookie")  = php::array(0);
	req->prop("body")   = nullptr;
	if(params.length() > 1) {
		req->prop("timeout") = params[1].to_long();
	}else{
		req->prop("timeout") = 2500;
	}
	return default_client->exec2(obj);
}

php::value post(php::parameters& params) {
	php::object     obj = php::object::create<client_request>();
	client_request* req = obj.native<client_request>();
	req->prop("method") = php::string("POST");
	req->prop("url")    = params[0];
	req->prop("header") = php::array(0);
	req->prop("cookie") = php::array(0);
	req->prop("body")   = params[1];
	if(params.length() > 2) {
		req->prop("timeout") = params[2].to_long();
	}else{
		req->prop("timeout") = 2500;
	}
	return default_client->exec2(obj);
	return flame::async();
}

php::value put(php::parameters& params) {
	php::object     obj = php::object::create<client_request>();
	client_request* req = obj.native<client_request>();
	req->prop("method") = php::string("PUT");
	req->prop("url")    = params[0];
	req->prop("header") = php::array(0);
	req->prop("cookie") = php::array(0);
	req->prop("body")   = params[1];
	if(params.length() > 2) {
		req->prop("timeout") = params[2].to_long();
	}else{
		req->prop("timeout") = 2500;
	}
	return default_client->exec2(obj);
}

php::value remove(php::parameters& params) {
	php::object     obj = php::object::create<client_request>();
	client_request* req = obj.native<client_request>();
	req->prop("method") = php::string("DELETE");
	req->prop("url")    = params[0];
	req->prop("header") = php::array(0);
	req->prop("cookie") = php::array(0);
	req->prop("body")   = nullptr;
	if(params.length() > 1) {
		req->prop("timeout") = params[1].to_long();
	}else{
		req->prop("timeout") = 2500;
	}
	return default_client->exec2(obj);
}

}
}
}
