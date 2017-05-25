#include "../../vendor.h"
#include "../../core.h"
#include "server.h"
#include "server_request.h"
#include "server_response.h"
#include "../init.h"
#include "init.h"

namespace net { namespace http {
	void server::init(php::extension_entry& extension) {
		php::class_entry<server> class_server("flame\\http\\server");
		class_server.add<&server::__destruct>("__destruct");
		class_server.add<&server::handle>("handle");
		class_server.add<&server::handle_default>("handle_default");
		class_server.add<&server::close>("close");
		class_server.add<&server::listen_and_serve>("listen_and_serve");
		extension.add(std::move(class_server));
	}

	server::server()
	: server_(evhttp_new(core::base))
	, closed_(true) {
		handler_default_.handler = php::value([this] (php::parameters& params) -> php::value {
			php::object        res_= params[1];
			server_response*   res = res_.native<server_response>();
			evbuffer*   buf = evbuffer_new();
			evbuffer_add_buffer_reference(buf, REPLY_NOT_FOUND);
			evhttp_send_reply(res->req_, 404, nullptr, buf);
			return nullptr;
		});
		handler_default_.self = this;
	}

	server::~server() {
		if(server_ != nullptr) evhttp_free(server_);
	}

	php::value server::handle_default(php::parameters& params) {
		if(params.length() > 0 && params[0].is_callable()) {
			handler_default_.handler = params[0];
			handler_default_.self = this;
		}else{
			throw php::exception("handle_default failed: handler must be a callable");
		}
	}

	php::value server::__destruct(php::parameters& params) {
		if(!cb_.is_empty()) { // 有可能已经调用过了
			cb_();
			// cb_.reset();
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
		evutil_socket_t fd = net::create_socket(local_addr_.va.sa_family, SOCK_STREAM, IPPROTO_TCP, true);
		if(::bind(fd,
			reinterpret_cast<struct sockaddr*>(&local_addr_),
			sizeof(local_addr_)) != 0) {
			throw php::exception(
				(boost::format("bind failed: %s") % strerror(errno)).str(), errno);
		}
		evconnlistener* listener_ = evconnlistener_new(
			core::base, nullptr, nullptr,
			LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE | LEV_OPT_DEFERRED_ACCEPT, 1024, fd);
		evconnlistener_set_error_cb(listener_, server::error_handler);
		// 默认处理逻辑
		evhttp_set_gencb(server_, server::request_handler, &handler_default_);
		// 初始化一些配置信息
		evhttp_set_default_content_type(server_, "text/plain");
		// TODO 提供 ini 配置？
		evhttp_set_max_headers_size(server_, 8*1024);
		evhttp_set_max_body_size(server_, 8*1024*1024);
		socket_ = evhttp_bind_listener(server_, listener_);
		if(socket_ == nullptr) {
			throw php::exception("listen failed: cannot listen on socket");
		}
		closed_ = false;
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
		handler_wrapper hw = handler_wrapper { params[1], this };
		handler_.push_back(hw);
		evhttp_set_cb(server_, path->val, server::request_handler, &handler_.back());
		return nullptr;
	}

	php::value server::close(php::parameters& params) {
		if(!closed_) {
			// 为平滑关闭，先摘除 socket_
			evhttp_del_accept_socket(server_, socket_);
			socket_ = nullptr;
			closed_ = true;
			// 有可能现在就没有需要处理的请求
			if(request_count_ == 0) {
				evhttp_free(server_);
				server_ = nullptr;
				if(!cb_.is_empty()) {
					cb_();
					cb_.reset();
				}
			}
		}
		return nullptr;
	}

	void server::request_handler(struct evhttp_request* evreq, void* ctx) {
		handler_wrapper* hw = reinterpret_cast<handler_wrapper*>(ctx);
		php::object     req_= php::object::create<server_request>();
		server_request* req = req_.native<server_request>();
		req->init(evreq, hw->self);
		php::object      res_= php::object::create<server_response>();
		server_response* res = res_.native<server_response>();
		res->init(evreq, hw->self);
		// 将构建的 request 及 response 对象回调给指定对应的 handler
		core::generator_start(hw->handler.invoke(req_, res_));
	}

	void server::request_finish() {
		--request_count_;
		// 所有活跃请求处理完毕，释放服务器
		if(request_count_ == 0 && closed_) {
			evhttp_free(server_);
			server_ = nullptr;
			if(!cb_.is_empty()) {
				cb_();
				cb_.reset();
			}
		}
	}

	void server::error_handler(struct evconnlistener *lis, void *ptr) {
		// 这里 ptr 实际指向的是 evhttp 对象无法获得当前 server
		int error = EVUTIL_SOCKET_ERROR();
		std::printf("socket error: %s\n", evutil_socket_error_to_string(error));
	}
}}
