#include "../../vendor.h"
#include "../../core.h"
#include "server.h"
#include "request.h"
#include "response.h"
#include "../init.h"
#include "init.h"

namespace net { namespace http {
	evbuffer* server::reply_not_found = nullptr;
	evbuffer* server::reply_internal_error = nullptr;
	void server::init(php::extension_entry& extension) {

		php::class_entry<server> class_server("flame\\http\\server");
		class_server.add<&server::handle>("handle");
		class_server.add<&server::close>("close");
		class_server.add<&server::listen_and_serve>("listen_and_serve");
		extension.add(std::move(class_server));
	}
	server::server()
	: server_(evhttp_new(core::base)) {
		handler_default_ = php::value([this] (php::parameters& params) -> php::value {
			php::object req_= params[0], res_ = params[1];
			response*   res = res_.native<response>();
			evbuffer*   buf = evbuffer_new();
			evbuffer_add_buffer_reference(buf, REPLY_NOT_FOUND);
			evhttp_send_reply(res->req_, 404, nullptr, buf);
			return nullptr;
		});
	}
	server::~server() {
		if(server_ != nullptr) {
			evhttp_free(server_);
		}
	}
	php::value server::listen_and_serve(php::parameters& params) {
		if(server_ == nullptr) throw php::exception("listen_and_serve failed: server already closed");
		int len = sizeof(local_addr_);
		zend_string* addr = params[0];
		net::parse_addr_port(
			addr->val, static_cast<int>(params[1]),
			reinterpret_cast<struct sockaddr*>(&local_addr_), &len
		);
		evutil_socket_t socket_ = net::create_socket(local_addr_.va.sa_family, SOCK_STREAM, IPPROTO_TCP, true);
		if(::bind(socket_,
			reinterpret_cast<struct sockaddr*>(&local_addr_),
			sizeof(local_addr_)) != 0) {
			throw php::exception(
				(boost::format("bind failed: %s") % strerror(errno)).str(), errno);
		}
		evconnlistener* listener_ = evconnlistener_new(
			core::base, nullptr, nullptr,
			LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE | LEV_OPT_DEFERRED_ACCEPT, 1024, socket_);
		evconnlistener_set_error_cb(listener_, server::error_handler);

		evhttp_set_gencb(server_, server::request_handler, &handler_default_);
		if(evhttp_bind_listener(server_, listener_) == nullptr) {
			throw php::exception("listen failed: cannot listen on socket");
		}
		return php::value([this] (php::parameters& params) -> php::value {
			cb_ = params[0];
			return nullptr;
		});
		return nullptr;
	}
	php::value server::local_addr(php::parameters& params) {
		php::string str(64);
		parse_addr(local_addr_.va.sa_family, reinterpret_cast<struct sockaddr*>(&local_addr_), str.data(), str.length());
		return std::move(str);
	}

	php::value server::local_port(php::parameters& params) {
		return ntohs(local_addr_.v4.sin_port); // v4 和 v6 sin_port 位置重合
	}

	php::value server::handle(php::parameters& params) {
		if(server_ == nullptr) throw php::exception("listen_and_serve failed: server already closed");
		zend_string* path = params[0];
		php::callable& handler = params[1];
		handler_.push_back(handler);
		evhttp_set_cb(server_, path->val, server::request_handler, &handler_.back());
		return nullptr;
	}

	php::value server::close(php::parameters& params) {
		evhttp_free(server_);
		server_ = nullptr;
		cb_();
		return nullptr;
	}

	void server::request_handler(struct evhttp_request* evreq, void* ctx) {
		php::callable* handler = reinterpret_cast<php::callable*>(ctx);
		php::object     req_= php::object::create<server_request>();
		server_request* req = req_.native<server_request>();
		req->req_ = evreq;
		php::object      res_= php::object::create<server_response>();
		server_response* res = res_.native<server_response>();
		res->req_ = evreq;
		evhttp_request_set_on_complete_cb(evreq, server_response::complete_handler, res);
		// 将构建的 request 及 response 对象回调给指定对应的 handler
		core::generator_start(handler->invoke(req_, res_));
	}

	void server::error_handler(struct evconnlistener *lis, void *ptr) {
		// 这里 ptr 实际指向的是 evhttp 对象无法获得当前 server
		int error = EVUTIL_SOCKET_ERROR();
		std::printf("socket error: %s\n", evutil_socket_error_to_string(error));
	}
}}
