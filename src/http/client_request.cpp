#include "../vendor.h"
#include "../core.h"
#include "init.h"
#include "header.h"
#include "client.h"
#include "client_request.h"
#include "client_response.h"

namespace http {
	void client_request::init(php::extension_entry& extension) {
		php::class_entry<client_request> class_client_request("flame\\http\\client_request");
		class_client_request.add<&client_request::__construct>("__construct");
		class_client_request.add(php::property_entry("header", nullptr));
		class_client_request.add(php::property_entry("body", std::string("")));
		extension.add(std::move(class_client_request));
        // ssl init
	}

	client_request::client_request()
	: req_(evhttp_request_new(client::complete_handler, this))
	, uri_(nullptr)
	, cli_(nullptr) 
    {
		evhttp_request_set_error_cb(req_, client::error_handler);
	}
	client_request::~client_request() {
		// 请求对象 req_ 交由 client_response 释放
		// evhttp_request_free(req_);
		if(uri_ != nullptr) {
			evhttp_uri_free(uri_);
		}
    }
    SSL* client_ssl_init() {
        int ret = 0;
        static bool ssl_init = false;
        if(!ssl_init) {
            ret = SSL_library_init();
            SSL_load_error_strings();
            ssl_init = true;
        }
        SSL_CTX* ssl_ctx_ = nullptr;
        if((ssl_ctx_ = SSL_CTX_new(SSLv23_method())) == nullptr) {
            throw php::exception("failed to SSL_CTX_new: unknown error");
        }
        if((ret = SSL_CTX_set_default_verify_paths(ssl_ctx_)) != 1) {
            throw php::exception("failed to SSL_CTX_set_default_verify_paths: unknown error");
        }
        SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_PEER, NULL);
        SSL* ssl_ = nullptr;
        if((ssl_ = SSL_new(ssl_ctx_)) == nullptr) {
            throw php::exception("failed to SSL_new: unknown error");
        }
        return ssl_;
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
            } else {
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
	int client_request::execute(client* cli) {
		cli_ = cli;
		// 准备连接
		conn_ = cli_->acquire(key_);
		if(conn_ == nullptr) {
            bufferevent* bev_ = nullptr;
            if(evutil_ascii_strncasecmp(evhttp_uri_get_scheme(uri_), "https", 5) == 0) {
                bev_ = bufferevent_openssl_socket_new(core::base, -1, client_ssl_init(),
                        BUFFEREVENT_SSL_CONNECTING, 
                        BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);
            }
			// 无可复用的连接，建立新连接
            conn_ = evhttp_connection_base_bufferevent_new(
                    core::base, core::base_dns, bev_, 
                    evhttp_uri_get_host(uri_), 
                    evhttp_uri_get_port(uri_));
		}
		evhttp_connection_set_closecb(conn_, client::close_handler, this);
		if(conn_ == nullptr) {
			return -1;
		}
		// 填充 body 内容到输出缓冲区
		php::string& body = prop("body");
        if(body.length() > 0) {
            evbuffer_add_reference(evhttp_request_get_output_buffer(req_), 
                    body.data(), body.length(), nullptr, nullptr);
        }
        std::string uri;
		// 请求地址
        if(evhttp_uri_get_query(uri_)) {
            uri = (boost::format("%s?%s")
                    % evhttp_uri_get_path(uri_) % evhttp_uri_get_query(uri_)).str();
        } else {
            uri = (boost::format("%s")
                    % evhttp_uri_get_path(uri_)).str();
        }
		if(-1 == evhttp_make_request(conn_, req_, cmd_, uri.c_str())) {
			return -2;
		}
		return 0;
	}
}
