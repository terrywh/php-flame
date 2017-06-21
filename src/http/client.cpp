#include "../vendor.h"
#include "../core.h"
#include "client.h"
#include "client_request.h"
#include "client_response.h"

namespace http {
	void client::init(php::extension_entry& extension) {
		php::class_entry<client> class_client("flame\\http\\client");
		class_client.add<&client::__construct>("__construct");
		class_client.add<&client::get>("get");
		class_client.add<&client::get>("post");
		class_client.add<&client::execute>("execute");
		extension.add(std::move(class_client));
	}
	client::client()
	: ttl_(15) {
		struct timeval to_ { 5, 0 };
		event_assign(&ev_, core::base, -1, EV_READ | EV_PERSIST, client::sweep_handler, this);
		event_add(&ev_, &to_);
	}
	client::~client() {
		event_del(&ev_);
	}
	php::value client::__construct(php::parameters& params) {
		if(params.length() > 0) {
			ttl_ = params[0];
		}
		return nullptr;
	}
	// 辅助构造 client_request 对象
	php::value client::get(php::parameters& params) {
		if(!params[0].is_string()) {
			throw php::exception("get failed: uri of type string is required");
		}
		php::object     req_= php::object::create<client_request>();
		client_request* req = req_.native<client_request>();
		php::value argc[2];
		argc[0] = php::string("GET", 3);
		argc[1] = params[0];
		php::parameters argv(2, reinterpret_cast<zval*>(argc));
		req->__construct(argv);
		// req_ 对象会在后面销毁，这里需要特殊处理一下
		return php::value([this, req_, req] (php::parameters& params) -> php::value {
			req->cb_ = params[0];
			if(req->execute(this) < 0) {
				php::callable cb = std::move(req->cb_);
				cb(php::make_exception("execute failed: unknown"));
			}
            reqs_.insert(std::pair<void*, php::object>(req, req_));
			// req->cb_ 等待 complete_handler 调用
			return nullptr;
		});
	}

	php::value client::post(php::parameters& params) {
		if(!params[0].is_string() || !params[1].is_string()) {
			throw php::exception("post failed: uri and post data of type string is required");
		}
		php::object     req_= php::object::create<client_request>();
		client_request* req = req_.native<client_request>();
		php::value argc[2];
		argc[0] = php::string("GET", 3);
		argc[1] = params[0];
		php::parameters argv(2, reinterpret_cast<zval*>(argc));
		req->__construct(argv);
		req->prop("body") = params[1];
		return php::value([this, req_, req] (php::parameters& params) -> php::value {
			req->cb_ = params[0];
			if(req->execute(this) < 0) {
				php::callable cb = std::move(req->cb_);
				cb(php::make_exception("execute failed: unknown"));
			}
			// req->cb_ 等待 complete_handler 调用
			return nullptr;
		});
	}
	//
	php::value client::execute(php::parameters& params) {
		php::object req_= params[0];
		if(!req_.is_instance_of<client_request>()) {
			throw php::exception("execute failed: object of 'flame\\net\\http\\client_request' required");
		}
		client_request* req = req_.native<client_request>();
		if(req->cli_ != nullptr) { // 以 cli_ 为标志，禁止对 request 对象重用
			throw php::exception("execute failed: reusing client_request");
		}
		return php::value([this, req_, req] (php::parameters& params) -> php::value {
			req->cb_ = params[0];
			if(req->execute(this) < 0) {
				php::callable cb = std::move(req->cb_);
				cb(php::make_exception("execute failed: unknown"));
			}
			// req->cb_ 等待 complete_handler 调用
			return nullptr;
		});
	}

	void client::complete_handler(struct evhttp_request* req_, void* ctx) {
		//std::printf("complete\n");
		client_request* req = reinterpret_cast<client_request*>(ctx);
		if(req->cb_.is_empty()) return; // 发生错误已经被处理过了
		client* self = req->cli_;
        php::callable cb = std::move(req->cb_);
		if(req_ == nullptr || evhttp_request_get_response_code(req_) == 0) { // 其他未知错误
			cb(php::make_exception("request failed: unknown error", 0));
			return;
		}
		// 构建 response 对象
		php::object res_= php::object::create<client_response>();
		client_response* res = res_.native<client_response>();
		res->init(req_);
		// 当请求完成后，连接还未释放，表示可以重用
		if(req->conn_ != nullptr) {
			self->release(req->key_, req->conn_);
			req->conn_ = nullptr;
		}
        self->reqs_.erase(req);
        // 返回 client_response 对象
		cb(nullptr, res_);
	}

	void client::error_handler(enum evhttp_request_error err, void* ctx) {
		std::printf("error\n");
		client_request* req = reinterpret_cast<client_request*>(ctx);
		std::string error = "request failed: ";
		php::callable cb = std::move(req->cb_);
		switch(err) {
		case EVREQ_HTTP_TIMEOUT:
			error.append("ETIMEOUT");
			cb(php::make_exception(error, ETIMEDOUT));
			break;
		case EVREQ_HTTP_INVALID_HEADER:
			break;
		case EVREQ_HTTP_EOF:
			error.append("EOF");
			cb(php::make_exception(error, -1));
			break;
		case EVREQ_HTTP_BUFFER_ERROR:
			error.append("buffer error");
			cb(php::make_exception(error, 0));
			break;
		case EVREQ_HTTP_REQUEST_CANCEL:
			cb(nullptr, nullptr);
			break;
		case EVREQ_HTTP_DATA_TOO_LONG:
			error.append("data overflow");
			cb(php::make_exception(error, EOVERFLOW));
			break;
		default:
			assert(0); // 理论上不可达的错误
		}
	}

	void client::close_handler(struct evhttp_connection* conn_, void* ctx) {
		client_request* req = reinterpret_cast<client_request*>(ctx);
		req->conn_ = nullptr;
	}

	evhttp_connection* client::acquire(const std::string& key) {
		auto ic = connection_.find(key);
		if(ic == connection_.end())	return nullptr;
		connection_.erase(ic);
		return ic->second.conn;
	}

	void client::release(const std::string& key, evhttp_connection* conn) {
		// TODO ini 配置下面常量？
		if(connection_.count(key) < 4) {
			connection_.insert(
				std::make_pair(key, (connection_wrapper){ttl_, conn})
			);
		}else{
            bufferevent* bev_ = evhttp_connection_get_bufferevent(conn);
            SSL* ssl_ = bufferevent_openssl_get_ssl(bev_);
            if(ssl_ != nullptr) {
                SSL_free(ssl_);
                SSL_CTX* ssl_ctx_ = SSL_get_SSL_CTX(ssl_);
                SSL_CTX_free(ssl_ctx_);
            }
			evhttp_connection_free(conn);
		}
	}

	void client::sweep_handler(evutil_socket_t fd, short events, void* ctx) {
		client* self = reinterpret_cast<client*>(ctx);
		for(auto i=self->connection_.begin();i!=self->connection_.end(); ) {
			i->second.ttl -= 5;
			if(i->second.ttl <= 0) {
                evhttp_connection_free(i->second.conn);
                i = self->connection_.erase(i);
			}else{
				++i;
			}
		}
	}
}
