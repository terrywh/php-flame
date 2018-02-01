#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../time/time.h"
#include "client.h"
#include "client_request.h"
#include "client_response.h"

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);
namespace flame {
namespace net {
namespace http {

php::value client::__construct(php::parameters& params) {
	if(params.length() == 0 || !params[0].is_array()) {
		return nullptr;
	}
	php::array&  opts = params[0];
	int host = opts.at("conn_per_host");
	if(host > 0 && host < 512) {
		curl_multi_setopt(multi_, CURLMOPT_MAX_HOST_CONNECTIONS, host);
	}else{
		curl_multi_setopt(multi_, CURLMOPT_MAX_HOST_CONNECTIONS, 2);
	}
	php::string& conn = opts.at("conn_share");
	if(conn.is_empty()) {
		curl_multi_setopt(multi_, CURLMOPT_PIPELINING, CURLPIPE_HTTP1 | CURLPIPE_MULTIPLEX); // 默认需要 pipeline
	}else if(conn.length() == 4 && std::strncmp(conn.c_str(), "none", 4) == 0) {
		curl_multi_setopt(multi_, CURLMOPT_PIPELINING, CURLPIPE_NOTHING);
	}else if(conn.length() == 4 && std::strncmp(conn.c_str(), "pipe", 4) == 0) {
		curl_multi_setopt(multi_, CURLMOPT_PIPELINING, CURLPIPE_HTTP1);
	}else if(conn.length() == 4 && std::strncmp(conn.c_str(), "plex", 4) == 0) {
		curl_multi_setopt(multi_, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
	}else if(conn.length() == 4 && std::strncmp(conn.c_str(), "both", 4) == 0) {
		curl_multi_setopt(multi_, CURLMOPT_PIPELINING, CURLPIPE_HTTP1 | CURLPIPE_MULTIPLEX);
	}
	int pipe = opts.at("pipe_per_conn");
	if(pipe > 0 && pipe < 256) {
		curl_multi_setopt(multi_, CURLMOPT_MAX_PIPELINE_LENGTH, pipe);
	}else{
		curl_multi_setopt(multi_, CURLMOPT_MAX_PIPELINE_LENGTH, 4);
	}
	return nullptr;
}
void client::default_options() {
	curl_multi_setopt(multi_, CURLMOPT_PIPELINING, CURLPIPE_HTTP1 | CURLPIPE_MULTIPLEX);
    curl_multi_setopt(multi_, CURLMOPT_MAX_PIPELINE_LENGTH, 4);
    curl_multi_setopt(multi_, CURLMOPT_MAX_HOST_CONNECTIONS, 2);
}
typedef struct exec_context_t {
	coroutine*       co;
	client*          self;
	php::object      req_obj;
	client_request*  req;
	php::object      res_obj;
	client_response* res;
} exec_context_t;

void client::curl_multi_info_check(client* self) {
	int pending;
	CURLMsg *message;
	while((message = curl_multi_info_read(self->multi_, &pending))) {
		if(message->msg == CURLMSG_DONE) {
			exec_context_t* ctx;
			curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &ctx);
			long status;
			curl_easy_getinfo(message->easy_handle, CURLINFO_RESPONSE_CODE, &status);
			assert(message->easy_handle == ctx->req->easy_);
			curl_multi_remove_handle(ctx->self->multi_, ctx->req->easy_);
			if (message->data.result != CURLE_OK) {
				ctx->co->fail(curl_easy_strerror(message->data.result), message->data.result);
			} else {
				ctx->res->done_cb(status);
				ctx->req->done_cb(message);  // 因为 req 的 done_cb 将关闭 easy_handle（导致所有 msg 数据无法访问）
				ctx->co->next(std::move(ctx->res));
			}
			delete ctx;
		}else{
			std::fprintf(stderr, "[%s] (flame\\net\\http\\client): error: message not done\n", time::datetime(time::now()));
		}
	}
}

void client::curl_multi_socket_poll(uv_poll_t *poll, int status, int events) {
	int flags = 0, running_handles;
	if(events & UV_READABLE)
		flags |= CURL_CSELECT_IN;
	if(events & UV_WRITABLE)
		flags |= CURL_CSELECT_OUT;
	client* self = reinterpret_cast<client*>(poll->data);
	int fd;
	uv_fileno((uv_handle_t*)poll, &fd);
	curl_multi_socket_action(self->multi_, fd, flags, &running_handles);
	curl_multi_info_check(self);
}

void client::curl_multi_timer_cb(uv_timer_t *req) {
	int running_handles;
	client* self = reinterpret_cast<client*>(req->data);
	curl_multi_socket_action(self->multi_, CURL_SOCKET_TIMEOUT, 0, &running_handles);
	curl_multi_info_check(self);
}

int client::curl_multi_timer_handle(CURLM* multi, long timeout_ms, void* userp) {
	if(userp) {
		client* self = reinterpret_cast<client*>(userp);
		if(timeout_ms < 0) {
			uv_timer_stop(self->timer_);
		} else {
			if(timeout_ms == 0) timeout_ms = 1;
			uv_timer_start(self->timer_, curl_multi_timer_cb, timeout_ms, 0);
		}
	}
	return 0;
}

int client::curl_multi_socket_handle(CURL* easy, curl_socket_t s, int action, void *userp, void *socketp) {
	int events = 0;
	client*    self = (client*)userp;
	uv_poll_t* poll = (uv_poll_t*)socketp;
	switch(action) {
	case CURL_POLL_IN:
	case CURL_POLL_OUT:
	case CURL_POLL_INOUT:
	{
		if(action != CURL_POLL_IN)  events |= UV_WRITABLE;
		if(action != CURL_POLL_OUT) events |= UV_READABLE;

		if(!poll) {
			poll = (uv_poll_t*)malloc(sizeof(uv_poll_t));
			uv_poll_init_socket(flame::loop, poll, s);
			poll->data = self;
			curl_multi_assign(self->multi_, s, poll);
		}
		uv_poll_start(poll, events, client::curl_multi_socket_poll);
	}
	break;
	case CURL_POLL_REMOVE:
		if(poll) {
			curl_multi_assign(self->multi_, s, nullptr);
			uv_poll_stop(poll);
			uv_close((uv_handle_t*)poll, flame::free_handle_cb);
		}
	break;
	default:
		assert(0);
	}
	return 0;
};

client::client()
: debug_(0) {
	multi_ = curl_multi_init();
	curl_multi_setopt(multi_, CURLMOPT_SOCKETFUNCTION, curl_multi_socket_handle);
	curl_multi_setopt(multi_, CURLMOPT_SOCKETDATA, this);
	curl_multi_setopt(multi_, CURLMOPT_TIMERFUNCTION, curl_multi_timer_handle);
	curl_multi_setopt(multi_, CURLMOPT_TIMERDATA, this);

	timer_ = (uv_timer_t*)malloc(sizeof(uv_timer_t));
	uv_timer_init(flame::loop, timer_);
	timer_->data = this;
}
size_t client::curl_easy_head_cb(char* ptr, size_t size, size_t nitems, void* userdata) {
	exec_context_t* ctx = reinterpret_cast<exec_context_t*>(userdata);
	size_t          len = size*nitems;
	ctx->res->head_cb(ptr, len);
	return len;
}
size_t client::curl_easy_body_cb(char* ptr, size_t size, size_t nmemb, void *userdata) {
	exec_context_t* ctx = reinterpret_cast<exec_context_t*>(userdata);
	size_t          len = size*nmemb;
	ctx->res->body_cb(ptr, len);
	return len;
}
php::value client::exec2(php::object& req_obj) {
	if(!req_obj.is_instance_of<client_request>()) {
		throw php::exception("only instanceof 'class client_request' can be executed");
	}
	client_request* req = req_obj.native<client_request>();
	if(req->easy_ == nullptr) {
		throw php::exception("request object can NOT be reused");
	}
	exec_context_t* ctx = new exec_context_t {
		coroutine::current, this,
		req_obj, req,
		php::object::create<client_response>()
	};
	ctx->res = ctx->res_obj.native<client_response>();
	ctx->req->build();

	curl_easy_setopt(req->easy_, CURLOPT_PRIVATE, ctx);
	curl_easy_setopt(req->easy_, CURLOPT_WRITEFUNCTION, curl_easy_body_cb);
	curl_easy_setopt(req->easy_, CURLOPT_WRITEDATA, ctx);
	curl_easy_setopt(req->easy_, CURLOPT_HEADERFUNCTION, curl_easy_head_cb);
	curl_easy_setopt(req->easy_, CURLOPT_HEADERDATA, ctx);

	curl_multi_add_handle(multi_, req->easy_);
	return flame::async();
}
php::value client::exec1(php::parameters& params) {
	php::object& obj = params[0];
	if(!obj.is_instance_of<client_request>()) {
		throw php::exception("object of type 'client_request' is required");
	}
	return exec2(obj);
}
client::~client() {
	if (multi_) {
		curl_multi_cleanup(multi_);
		multi_ = nullptr;
	}
	if(timer_) {
		uv_timer_stop(timer_);
		uv_close((uv_handle_t*)timer_, free_handle_cb);
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
		req->prop("timeout") = 3000;
	}
	return default_client->exec2(obj);
}
php::value post(php::parameters& params) {
	php::object     obj = php::object::create<client_request>();
	// client_request* req = obj.native<client_request>();
	obj.prop("method") = php::string("POST");
	obj.prop("url")    = params[0];
	obj.prop("header") = php::array(0);
	obj.prop("cookie") = php::array(0);
	obj.prop("body")   = params[1];
	if(params.length() > 2) {
		obj.prop("timeout") = params[2].to_long();
	}else{
		obj.prop("timeout") = 3000;
	}
	return default_client->exec2(obj);
}
php::value put(php::parameters& params) {
	php::object     obj = php::object::create<client_request>();
	client_request* req = obj.native<client_request>();
	obj.prop("method") = php::string("PUT");
	obj.prop("url")    = params[0];
	obj.prop("header") = php::array(0);
	obj.prop("cookie") = php::array(0);
	obj.prop("body")   = params[1];
	if(params.length() > 2) {
		obj.prop("timeout") = params[2].to_long();
	}else{
		obj.prop("timeout") = 3000;
	}
	return default_client->exec2(obj);
}
php::value remove(php::parameters& params) {
	php::object     obj = php::object::create<client_request>();
	client_request* req = obj.native<client_request>();
	obj.prop("method") = php::string("DELETE");
	obj.prop("url")    = params[0];
	obj.prop("header") = php::array(0);
	obj.prop("cookie") = php::array(0);
	obj.prop("body")   = nullptr;
	if(params.length() > 1) {
		obj.prop("timeout") = params[1].to_long();
	}else{
		obj.prop("timeout") = 3000;
	}
	return default_client->exec2(obj);
}
php::value exec(php::parameters& params) {
	php::object& obj = params[0];
	if(!obj.is_instance_of<client_request>()) {
		throw php::exception("object of type 'client_request' is required");
	}
	return default_client->exec2(obj);
}

}
}
}
