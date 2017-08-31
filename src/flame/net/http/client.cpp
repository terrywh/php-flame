#include "../../fiber.h"
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
}

void client::curl_check_multi_info(client* self) {
	int pending;
	CURLMsg *message;
	while((message = curl_multi_info_read(self->curlm_, &pending))) {
		client_request* req;
		CURL* easy_handle = message->easy_handle;
		curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &req);
		req->cb_(message);
		curl_multi_remove_handle(self->curlm_, easy_handle);
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
	curl_socket_t fd = reinterpret_cast<client_request*>(req->data)->curl_fd;
	client* self = reinterpret_cast<client_request*>(req->data)->cli_;
	curl_multi_socket_action(self->curlm_, fd, flags, &running_handles);
	curl_check_multi_info(self);
}

void client::curl_timeout_cb(uv_timer_t *req) {
	int running_handles;
	client* self = reinterpret_cast<client*>(req->data);
	curl_multi_socket_action(self->curlm_, CURL_SOCKET_TIMEOUT, 0, &running_handles);
	curl_check_multi_info(self);
}

int client::curl_start_timeout(CURLM* multi, long timeout_ms, void* userp) {
	if(userp) {
		client* self = reinterpret_cast<client*>(userp);
		if(timeout_ms < 0) {
			uv_timer_stop(&self->timer_);
		} else {
			if(timeout_ms == 0) timeout_ms = 1;
			uv_timer_start(&self->timer_, curl_timeout_cb, timeout_ms, 0);
		}
	}
	return 0;
}

int client::curl_handle_socket(CURL* easy, curl_socket_t s, int action, void *userp, void *socketp) {
	client_request* req;
	int events = 0;

	switch(action) {
	case CURL_POLL_IN:
	case CURL_POLL_OUT:
	case CURL_POLL_INOUT:
	{
		curl_easy_getinfo(easy, CURLINFO_PRIVATE, &req);
		if(action != CURL_POLL_IN) events |= UV_WRITABLE;
		if(action != CURL_POLL_OUT) events |= UV_READABLE;
		if(socketp) {
			req = reinterpret_cast<client_request*>(socketp);
		}
		else {
			req->curl_fd = s;
			uv_poll_init_socket(flame::loop, &req->poll_, s);
			req->poll_.data = req;
			curl_multi_assign(req->cli_->curlm_, s, req);
		}
		uv_poll_start(&req->poll_, events, client::curl_perform);
	}
	break;
	case CURL_POLL_REMOVE:
		curl_easy_getinfo(easy, CURLINFO_PRIVATE, &req);
		if(socketp) {
			uv_poll_t* poll = &reinterpret_cast<client_request*>(socketp)->poll_;
			uv_poll_stop(poll);
			uv_close(reinterpret_cast<uv_handle_t*>(poll), nullptr);
			curl_multi_assign(req->cli_->curlm_, s, nullptr);
		}
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
	curl_multi_setopt(curlm_, CURLMOPT_SOCKETFUNCTION, curl_handle_socket);
	curl_multi_setopt(curlm_, CURLMOPT_TIMERFUNCTION, curl_start_timeout);
	curl_multi_setopt(curlm_, CURLMOPT_TIMERDATA, this);
}

php::value client::exec(php::object& req_obj) {
	if(!req_obj.is_instance_of<client_request>()) {
		throw php::exception("only instanceof 'class client_request' can be executed");
	}
	client_request* req = req_obj.native<client_request>();
	req->build(this);
	if (debug_) {
		curl_easy_setopt(req->curl_, CURLOPT_VERBOSE, 1L);
	}
	auto fib = flame::this_fiber()->push();
	req->cb_ = [req_obj, req, fib](CURLMsg *message) {
		switch(message->msg) {
		case CURLMSG_DONE:
			if (message->data.result != CURLE_OK) {
				fib->next(php::make_exception(curl_easy_strerror(message->data.result), message->data.result));
			} else {
				fib->next(std::move(req->buffer_));
			}
		break;
		default:
			assert(0);
		}
	};
	curl_multi_add_handle(curlm_, req->curl_);
	return flame::async();
}

php::value client::debug(php::parameters& params) {
	debug_ = params[0];
	return params[0];
}

php::value client::exec(php::parameters& params) {
	php::object& obj = params[0];
	if(!obj.is_instance_of<client_request>()) {
		php::exception("object of type 'client_request' is required");
	}
	return exec(obj);
}

void client::release() {
	if (curlm_) {
		uv_timer_stop(&timer_);
		curl_multi_cleanup(curlm_);
		curlm_ = nullptr;
	}
}

client* default_client = nullptr;

php::value get(php::parameters& params) {
	php::object obj_req = php::object::create<client_request>();
	client_request* req = obj_req.native<client_request>();
	req->prop("method") = php::string("GET");
	req->prop("url")    = params[0];
	req->prop("header") = php::array();
	return default_client->exec(obj_req);
}

php::value post(php::parameters& params) {
	php::object obj_req = php::object::create<client_request>();
	client_request* req = obj_req.native<client_request>();
	req->prop("method") = php::string("POST");
	req->prop("url")    = params[0];
	req->prop("header") = php::array();
	req->prop("body")   = params[1];
	return default_client->exec(obj_req);
}

php::value put(php::parameters& params) {
	php::object obj_req = php::object::create<client_request>();
	client_request* req = obj_req.native<client_request>();
	req->prop("method") = php::string("PUT");
	req->prop("url")    = params[0];
	req->prop("header") = php::array();
	req->prop("body")   = params[1];
	return default_client->exec(obj_req);
}

}
}
}
