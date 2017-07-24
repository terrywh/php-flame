#include <phpext.h>
#include <typeinfo>
#include <iostream>
#include "http_client.h"
#include "../fiber.h"

#define MY_DEBUG

// 所有导出到 PHP 的函数必须符合下面形式：
// php::value fn(php::parameters& params);
namespace flame {
namespace net {

size_t write_callback(char* ptr, size_t size, size_t nmemb, void *userdata)
{
	http_client* pClient = (http_client*)userdata;
	size_t ret_len = size*nmemb > CURL_MAX_WRITE_SIZE ? CURL_MAX_WRITE_SIZE : size*nmemb;
	if(pClient) {
		pClient->SetResult(ptr, ret_len);
	}
	else {
		assert(0);
	}
	return ret_len;
}


static void check_multi_info(http_client* pClient)
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
			flame::fiber*  f = pClient->_fiber;
			f->next(pClient->_result);
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
			flame::fiber*  f = pClient->_fiber;
			f->next(pClient->_result);
		}
		break;
		}
	}
}

void http_client::curl_perform(uv_poll_t *req, int status, int events)
{
	int flags = 0;
	int running_handles;
	if(events & UV_READABLE)
		flags |= CURL_CSELECT_IN;
	if(events & UV_WRITABLE)
		flags |= CURL_CSELECT_OUT;
	curl_socket_t fd = ((curl_context_t*)req->data)->sockfd;
	http_client* pClient = ((curl_context_t*)req->data)->client;
	curl_multi_socket_action(pClient->get_curl_handle(), fd, flags, &running_handles);
	check_multi_info(pClient);
}


static void on_timeout(uv_timer_t *req)
{
	int running_handles;
	http_client* pClient = (http_client*)(req->data);
	curl_multi_socket_action(pClient->get_curl_handle(), CURL_SOCKET_TIMEOUT, 0, &running_handles);
	check_multi_info(pClient);
}

int http_client::start_timeout(CURLM* multi, long timeout_ms, void* userp)
{
	if(userp) {
		http_client* pClient = (http_client*)userp;
		if(timeout_ms < 0) {
			uv_timer_stop(&pClient->_timeout);
		}
		else {
			if(timeout_ms == 0)
				timeout_ms = 1;
			pClient->_timeout.data = pClient;
			uv_timer_start(&pClient->_timeout, on_timeout, timeout_ms, 0);
		}
	}
	return 0;
}


static void curl_close_cb(uv_handle_t *handle)
{
	delete (curl_context_t*)handle->data;
}

php::value http_request::__construct(php::parameters& params)
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
				php::array& post = arr["_POST"];
				if (!post.is_empty()) {
					_postfield = build_str(post);
				}
			}
		} else if( arg_len == 3) {
			php::string& method = params[0];
			prop("method") = method;
			php::string& url = params[1];
			prop("url") = url;
			php::array& content = params[2];
			_postfield = build_str(content);
		}
	}
	php::array arr(0);
	prop("header") = arr;
	return this;
}


php::value http_client::__construct(php::parameters& params)
{
	if(curl_global_init(CURL_GLOBAL_ALL)) {
		throw php::exception("curl init error", -1);
		return nullptr;
	}
#ifdef MY_DEBUG
	_debug = 1;
#endif
	return this;
}

int http_client::handle_socket(CURL* easy, curl_socket_t s, int action, void *userp, void *socketp)
{
	http_client *pClient;
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
			pContext->client = pClient;
		}
		curl_multi_assign(pClient->get_curl_handle(), s, (void*)pContext);
		uv_poll_init_socket(flame::loop, &pContext->poll_handle, s);
		pContext->poll_handle.data = pContext;
		uv_poll_start(&pContext->poll_handle, events, http_client::curl_perform);
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

php::value http_client::exec(http_request* pRequest)
{
	if (ExeOnce() == 0) {
		_curlm_handle = curl_multi_init();
		uv_timer_init(flame::loop, &_timeout);
		curl_multi_setopt(_curlm_handle, CURLMOPT_SOCKETFUNCTION, handle_socket);
		curl_multi_setopt(_curlm_handle, CURLMOPT_TIMERFUNCTION, start_timeout);
		curl_multi_setopt(_curlm_handle, CURLMOPT_TIMERDATA, (void*)this);
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
	if (_debug) {
#endif
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	}
	php::string str = pRequest->get_method();
	if(str.is_empty() == false) {
		std::string method(str.c_str());
		if(method.compare("POST") == 0) {
			curl_easy_setopt(curl, CURLOPT_POST, 1L);
			if (pRequest->_postfield.is_null()) {
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
			} else {
				curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, pRequest->_postfield.c_str());
			}
		}else if(method.compare("PUT") == 0) {
			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
			curl_easy_setopt(curl, CURLOPT_PUT, 1L);
			//curl_easy_setopt(curl, CURLOPT_READDATA, pRequest->_postfield.c_str());
		}else if(method.compare("GET") == 0) {
		}
	}
	if (flame::this_fiber() == NULL) {
		throw php::exception("need yield!", -1);
	} else {
		_fiber = flame::this_fiber();
	}

	curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)pRequest->get_timeout());
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)this);
	curl_easy_setopt(curl, CURLOPT_PRIVATE, (void*)this);
	curl_multi_add_handle(get_curl_handle(), curl);
	return flame::async;	
}

php::value http_client::debug(php::parameters& params)
{
	_debug = params[0];
	return params[0];
}

php::value http_client::exec(php::parameters& params)
{
	php::object& obj = params[0];
	http_request* pRequest = obj.native<http_request>();
	if(pRequest == NULL) {
		return nullptr;
	}
	return exec(pRequest);
}


php::value http_get(php::parameters& params)
{
	static php::object obj = php::object::create<http_client>();
	http_client* client = obj.native<http_client>();
	php::object obj_req = php::object::create<http_request>();
	http_request* request = obj_req.native<http_request>();
	php::string url = params[0];
	request->prop("header") = php::array();
	request->prop("url") = url;
	request->prop("method") = php::string("GET");
	php::value ret =  client->exec(request);
	return ret;
}

php::value http_post(php::parameters& params)
{
	static php::object obj = php::object::create<http_client>();
	http_client* client = obj.native<http_client>();
	php::object obj_req = php::object::create<http_request>();
	http_request* request = obj_req.native<http_request>();
	php::string url = params[0];
	request->prop("url") = url;
	request->prop("method") = php::string("POST");
	request->prop("header") = php::array();
	php::array& arr = params[1];
	request->_postfield = php::build_str(arr);
	php::value ret =  client->exec(request);
	return ret;
}

php::value http_put(php::parameters& params)
{
	static php::object obj = php::object::create<http_client>();
	http_client* client = obj.native<http_client>();
	php::object obj_req = php::object::create<http_request>();
	http_request* request = obj_req.native<http_request>();
	php::string url = params[0];
	request->prop("url") = url;
	request->prop("method") = php::string("PUT");
	request->prop("header") = php::array();
	php::array& arr = params[1];
	request->_postfield = php::build_str(arr);
	php::value ret =  client->exec(request);
	return ret;
}

}
}
