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

static void check_multi_info(client* cli) {
	int pending;
	CURLMsg *message;
	while((message = curl_multi_info_read(cli->get_curl_handle(), &pending))) {
		client_request* req;
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
	curl_socket_t fd = reinterpret_cast<client_request*>(req->data)->sockfd_;
	client* cli = reinterpret_cast<client_request*>(req->data)->cli_;
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

client::client()
: debug_(0) {
	curlm_handle_ = curl_multi_init();
	uv_timer_init(flame::loop, &timeout_);
	curl_multi_setopt(curlm_handle_, CURLMOPT_SOCKETFUNCTION, handle_socket);
	curl_multi_setopt(curlm_handle_, CURLMOPT_TIMERFUNCTION, start_timeout);
	curl_multi_setopt(curlm_handle_, CURLMOPT_TIMERDATA, (void*)this);
}

CURLM* client::get_curl_handle() {
	return curlm_handle_;
}

int client::handle_socket(CURL* easy, curl_socket_t s, int action, void *userp, void *socketp) {
	client_request* req;
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
			req = reinterpret_cast<client_request*>(socketp);
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
			uv_poll_t* p_poll_handle = &reinterpret_cast<client_request*>(socketp)->poll_handle_;
			uv_poll_stop(p_poll_handle);
			uv_close((uv_handle_t*)p_poll_handle, nullptr);
			curl_multi_assign(req->cli_->get_curl_handle(), s, nullptr);
		}
	break;
	default:
		//不可能
		;
	}
	return 0;
};

php::value client::exec(php::object& req) {
	client_request* r = req.native<client_request>();
	r->build(this);
	CURL* curl = r->curl_;
	if(!curl) {
		throw php::exception("curl empty");
	}
	if (debug_) {
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	}
	auto fb = flame::this_fiber()->push();
	r->cb_ = [req, r, fb](CURLMsg *message) {
		switch(message->msg) {
		case CURLMSG_DONE:
			if (message->data.result != CURLE_OK) {
				php::string str_ret(curl_easy_strerror(message->data.result));
				fb->next(std::move(str_ret));
			} else {
				fb->next(std::move(r->result_));
			}
		break;
		default:
			fb->next(php::string("curlmsg return not zero"));
		}
	};
	curl_multi_add_handle(get_curl_handle(), curl);
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
	if (curlm_handle_) {
		uv_timer_stop(&timeout_);
		curl_multi_cleanup(curlm_handle_);
		curlm_handle_ = nullptr;
	}
}

php::value get(php::parameters& params) {
	static client cli;
	php::object obj_req = php::object::create<client_request>();
	client_request* req = obj_req.native<client_request>();
	req->prop("method") = php::string("GET");
	req->prop("url")    = params[0];
	req->prop("header") = php::array();
	return cli.exec(obj_req);
}

php::value post(php::parameters& params) {
	static client cli;
	php::object obj_req = php::object::create<client_request>();
	client_request* req = obj_req.native<client_request>();
	req->prop("method") = php::string("POST");
	req->prop("url") = params[0];
	req->prop("header") = php::array();
	req->prop("body")   = params[1];
	return cli.exec(obj_req);
}

php::value put(php::parameters& params) {
	static client cli;
	php::object obj_req = php::object::create<client_request>();
	client_request* req = obj_req.native<client_request>();
	req->prop("method") = php::string("PUT");
	req->prop("url")    = params[0];
	req->prop("header") = php::array();
	req->prop("body")   = params[1];
	return cli.exec(obj_req);
}

}
}
}
