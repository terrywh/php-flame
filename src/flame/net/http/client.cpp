#include "client.h"

//#define MY_DEBUG

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);
namespace flame {
namespace net {
namespace http {

size_t write_callback(char* ptr, size_t size, size_t nmemb, void *userdata)
{
	client* pClient = (client*)userdata;
	size_t ret_len = size*nmemb > CURL_MAX_WRITE_SIZE ? CURL_MAX_WRITE_SIZE : size*nmemb;
	if(pClient) {
		pClient->SetResult(ptr, ret_len);
	}
	else {
		assert(0);
	}
	return ret_len;
}


static void check_multi_info(client* pClient)
{
	int pending;
	CURLMsg *message;
	while((message = curl_multi_info_read(pClient->get_curl_handle(), &pending))) {
		switch(message->msg) {
		case CURLMSG_DONE:
		{
#ifdef MY_DEBUG
			printf("DONE\n");
#endif
			CURL* easy_handle = message->easy_handle;
			curl_multi_remove_handle(pClient->get_curl_handle(), easy_handle);
			curl_easy_cleanup(easy_handle);
			flame::fiber*  f = pClient->fiber_;
			f->next(pClient->result_);
		}
		break;
		default:
		{
#ifdef MY_DEBUG
			printf("CURLMsg default\n");
#endif
			CURL* easy_handle = message->easy_handle;
			curl_multi_remove_handle(pClient->get_curl_handle(), easy_handle);
			curl_easy_cleanup(easy_handle);
			flame::fiber*  f = pClient->fiber_;
			f->next(pClient->result_);
		}
		break;
		}
	}
}

void client::curl_perform(uv_poll_t *req, int status, int events)
{
	int flags = 0;
	int running_handles;
	if(events & UV_READABLE)
		flags |= CURL_CSELECT_IN;
	if(events & UV_WRITABLE)
		flags |= CURL_CSELECT_OUT;
	curl_socket_t fd = ((curl_context_t*)req->data)->sockfd;
	client* pClient = ((curl_context_t*)req->data)->cli;
	curl_multi_socket_action(pClient->get_curl_handle(), fd, flags, &running_handles);
	check_multi_info(pClient);
}


static void ontimeout_(uv_timer_t *req)
{
	int running_handles;
	client* pClient = (client*)(req->data);
	curl_multi_socket_action(pClient->get_curl_handle(), CURL_SOCKET_TIMEOUT, 0, &running_handles);
	check_multi_info(pClient);
}

int client::starttimeout_(CURLM* multi, long timeout_ms, void* userp)
{
	if(userp) {
		client* pClient = (client*)userp;
		if(timeout_ms < 0) {
			uv_timer_stop(&pClient->timeout_);
		}
		else {
			if(timeout_ms == 0)
				timeout_ms = 1;
			pClient->timeout_.data = pClient;
			uv_timer_start(&pClient->timeout_, ontimeout_, timeout_ms, 0);
		}
	}
	return 0;
}


static void curl_close_cb(uv_handle_t *handle)
{
	delete (curl_context_t*)handle->data;
}

php::value request::__construct(php::parameters& params)
{
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
				prop("timeout") = arr["timeout"];
				prop("header") = arr["header"];
				php::value& vpost = arr["_POST"];
				if (!vpost.is_null()) {
					php::array& post = arr["_POST"];
					if (!post.is_empty()) {
						postfield_ = build_str(post);
					}
				}
			}
		} else if( arg_len == 3) {
			php::string& method = params[0];
			prop("method") = method;
			php::string& url = params[1];
			prop("url") = url;
			php::array& content = params[2];
			postfield_ = build_str(content);
		}
	}
	php::array arr(0);
	prop("header") = arr;
	return this;
}


php::value client::__construct(php::parameters& params)
{
	if(curl_global_init(CURL_GLOBAL_ALL)) {
		throw php::exception("curl init error", -1);
		return nullptr;
	}
#ifdef MY_DEBUG
	debug_ = 1;
#endif
	return this;
}

int client::handle_socket(CURL* easy, curl_socket_t s, int action, void *userp, void *socketp)
{
	client *pClient;
	int events = 0;
	
	switch(action) {
	case CURL_POLL_IN:
	case CURL_POLL_OUT:
	case CURL_POLL_INOUT:
	{
		curl_easy_getinfo(easy, CURLINFO_PRIVATE, &pClient);
		if(action != CURL_POLL_IN)
			events |= UV_WRITABLE;
		if(action != CURL_POLL_OUT)
			events |= UV_READABLE;
		curl_context_t* pContext;
		if(socketp) {
			pContext = (curl_context_t*)socketp;
		}
		else {
			pContext = new curl_context_t;
			if (pContext == nullptr) {
				assert(0);
			}
			pContext->sockfd = s;
			pContext->cli = pClient;
		}
		curl_multi_assign(pClient->get_curl_handle(), s, (void*)pContext);
		uv_poll_init_socket(flame::loop, &pContext->poll_handle, s);
		pContext->poll_handle.data = pContext;
		uv_poll_start(&pContext->poll_handle, events, client::curl_perform);
	}
	break;
	case CURL_POLL_REMOVE:
		curl_easy_getinfo(easy, CURLINFO_PRIVATE, &pClient);
		if(socketp) {
			uv_poll_t* p_poll_handle = &((curl_context_t*)socketp)->poll_handle;
			uv_poll_stop(p_poll_handle);
			curl_multi_assign(pClient->get_curl_handle(), s, NULL);
			uv_close((uv_handle_t*)p_poll_handle, curl_close_cb);
			
		}
		break;
	default:
		//不可能
		break;
	}
	return 0;
};

php::value client::exec(request* pRequest)
{
	if (ExeOnce() == 0) {
		curlm_handle_ = curl_multi_init();
		uv_timer_init(flame::loop, &timeout_);
		curl_multi_setopt(curlm_handle_, CURLMOPT_SOCKETFUNCTION, handle_socket);
		curl_multi_setopt(curlm_handle_, CURLMOPT_TIMERFUNCTION, starttimeout_);
		curl_multi_setopt(curlm_handle_, CURLMOPT_TIMERDATA, (void*)this);
	}
	CURL* curl = curl_easy_init();
	if(curl == NULL) {
		throw php::exception("curl_easy_init fail", -1);
		return nullptr;
	}
	php::array header = pRequest->get_header();
	if(header.is_empty() == false) {
		struct curl_slist *p_slist = NULL;
		std::string str;
		for(auto i=header.begin();i!=header.end();++i) {
			php::string& key = i->first;
			php::string& value = i->second;
			if (value.length() > 0) {
				std::string str(key.c_str());
				str.append(": ");
				str.append(value.c_str());
				p_slist = curl_slist_append(p_slist, str.c_str());
			}
		}
		CURLcode ret = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, p_slist);
	}
	// 设置URL
	php::string& url = pRequest->prop("url");
	if (url.is_empty() == false) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	} else {
		// 没有url
		throw php::exception("url empty", -1);
		return nullptr;
	}
#ifdef MY_DEBUG
	if (true) {
#else
	if (debug_) {
#endif
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	}
	php::string str = pRequest->get_method();
	if(str.is_empty() == false) {
		std::string method(str.c_str());
		if(method.compare("POST") == 0) {
			curl_easy_setopt(curl, CURLOPT_POST, 1L);
			if (pRequest->postfield_.is_null()) {
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
			} else {
				curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, pRequest->postfield_.c_str());
			}
		}else if(method.compare("PUT") == 0) {
			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
			curl_easy_setopt(curl, CURLOPT_PUT, 1L);
			//curl_easy_setopt(curl, CURLOPT_READDATA, pRequest->postfield_.c_str());
		}else if(method.compare("GET") == 0) {
		}
	}
	fiber_ = flame::this_fiber();

	curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)pRequest->gettimeout_());
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)this);
	curl_easy_setopt(curl, CURLOPT_PRIVATE, (void*)this);
	curl_multi_add_handle(get_curl_handle(), curl);
	return flame::async;	
}

php::value client::debug(php::parameters& params)
{
	debug_ = params[0];
	return params[0];
}

php::value client::exec(php::parameters& params)
{
	php::object& obj = params[0];
	request* pRequest = obj.native<request>();
	if(pRequest == NULL) {
		return nullptr;
	}
	return exec(pRequest);
}


php::value get(php::parameters& params)
{
	static php::object obj = php::object::create<client>();
	client* cli = obj.native<client>();
	php::object obj_req = php::object::create<request>();
	request* req = obj_req.native<request>();
	php::string url = params[0];
	req->prop("header") = php::array();
	req->prop("url") = url;
	req->prop("method") = php::string("GET");
	php::value ret =  cli->exec(req);
	return ret;
}

php::value post(php::parameters& params)
{
	static php::object obj = php::object::create<client>();
	client* cli = obj.native<client>();
	php::object obj_req = php::object::create<request>();
	request* req = obj_req.native<request>();
	php::string url = params[0];
	req->prop("url") = url;
	req->prop("method") = php::string("POST");
	req->prop("header") = php::array();
	php::array& arr = params[1];
	req->postfield_ = php::build_str(arr);
	php::value ret =  cli->exec(req);
	return ret;
}

php::value put(php::parameters& params)
{
	static php::object obj = php::object::create<client>();
	client* cli = obj.native<client>();
	php::object obj_req = php::object::create<request>();
	request* req = obj_req.native<request>();
	php::string url = params[0];
	req->prop("url") = url;
	req->prop("method") = php::string("PUT");
	req->prop("header") = php::array();
	php::array& arr = params[1];
	req->postfield_ = php::build_str(arr);
	php::value ret =  cli->exec(req);
	return ret;
}


}
}
}
