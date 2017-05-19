#include "../../vendor.h"
#include "../../core.h"
#include "client.h"
#include "client_request.h"


namespace net { namespace http {
	void client::init(php::extension_entry& extension) {
		php::class_entry<client> class_client("flame\\net\\http\\client");
		class_client.add<&client::__construct>("__construct");
		class_client.add<&client::get>("get");
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

		return nullptr;
	}
	//
	php::value client::execute(php::parameters& params) {
		php::object req_= params[0];
		if(!req_.is_instance_of<client_request>()) {
			throw php::exception("execute failed: object of 'flame\\net\\http\\client_request' required");
		}
		client_request* req = req_.native<client_request>();
		return req->execute(this);
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
}}
