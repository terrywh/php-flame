#include "../../vendor.h"
#include "../../core.h"
#include "init.h"
#include "header.h"
#include "client.h"
#include "client_request.h"
#include "client_response.h"

namespace net { namespace http {
	void client_request::init(php::extension_entry& extension) {
		php::class_entry<client_request> class_client_request("flame\\net\\http\\client_request");
		class_client_request.add<&client_request::__construct>("__construct");
		extension.add(std::move(class_client_request));
	}

	client_request::client_request()
	: req_(evhttp_request_new(client_request::complete_handler, this))
	, uri_(nullptr) {

	}
	client_request::~client_request() {
		// 请求对象 req_ 交由 client_response 释放
		// evhttp_request_free(req_);
		if(uri_ != nullptr) {
			evhttp_uri_free(uri_);
		}
	}

	php::value client_request::__construct(php::parameters& params) {
		// METHOD
		php::string& method = params[0];
		cmd_ = method2command(method);
		// URI
		zend_string* uri = params[1];
		uri_ = evhttp_uri_parse(uri->val);
		if(uri_ == nullptr) {
			throw php::exception("failed to create client_request: illegal uri");
		}
		if(evhttp_uri_get_port(uri_) == -1) {
			if(evutil_ascii_strncasecmp(evhttp_uri_get_scheme(uri_), "https", 5) == 0) {
				evhttp_uri_set_port(uri_, 443);
			}else{
				evhttp_uri_set_port(uri_, 80);
			}
		}
		// 连接标识
		key_ = (boost::format("%s:%d") % evhttp_uri_get_host(uri_) %
			evhttp_uri_get_port(uri_)).str();
		// HEADER
		php::object hdr_= php::object::create<header>();
		hdr_.native<header>()->init(evhttp_request_get_output_headers(req_));
		// 需要使用 keep-alive 请自行指定 connection: keep-alive 头
		prop("header") = hdr_;
		return nullptr;
	}

	void client_request::complete_handler(struct evhttp_request* req_, void* ctx) {
		client_request* self = reinterpret_cast<client_request*>(ctx);
		// 构建 response 对象
		php::object res_= php::object::create<client_response>();
		client_response* res = res_.native<client_response>();
		res->init(self->req_);
		// 当请求完成后，连接还未释放，表示可以重用
		if(self->conn_ != nullptr) {
			self->cli_->release(self->key_, self->conn_);
		}
		// 返回 client_response 对象
		php::callable cb = std::move(self->cb_);
		cb(nullptr, res_);
	}

	php::value client_request::execute(client* cli) {
		cli_ = cli;
		return php::value([this] (php::parameters& params) -> php::value {
			cb_ = params[0];
			conn_ = cli_->acquire(key_);
			if(conn_ == nullptr) {
				// 无可复用的连接，建立新连接
				conn_ = evhttp_connection_base_new(
					core::base, core::base_dns,
					evhttp_uri_get_host(uri_),
					evhttp_uri_get_port(uri_)
				);
			}
			evhttp_connection_set_closecb(conn_, client_request::close_handler, this);
			if(conn_ == nullptr) {
				// !!! 此种错误发生的概率也不会很大
				php::callable cb = std::move(cb_);
				cb(php::make_exception("execute failed: failed to create connection", 0));
				return nullptr;
			}
			std::string uri = (boost::format("%s?%s")
				% evhttp_uri_get_path(uri_) % evhttp_uri_get_query(uri_)).str();
			if(-1 == evhttp_make_request(conn_, req_, cmd_, uri.c_str())) {
				// !!! 此种错误应该不太可能发生？
				cli_->release(key_, conn_);
				php::callable cb = std::move(cb_);
				cb(php::make_exception("execute failed", 0));
			}
			return nullptr;
		});
	}

	void client_request::close_handler(struct evhttp_connection* conn_, void* ctx) {
		client_request* self = reinterpret_cast<client_request*>(ctx);
		self->conn_ = nullptr;
	}
}}
