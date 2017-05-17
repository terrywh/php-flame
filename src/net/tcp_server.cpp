#include "../vendor.h"
#include "tcp_server.h"
#include "../core.h"
#include "init.h"
#include "tcp_socket.h"

namespace net {

	void tcp_server::init(php::extension_entry& extension) {
		php::class_entry<tcp_server> class_tcp_server("flame\\net\\tcp_server");
		// class_tcp_server.add<&tcp_server::__construct>("__construct");
		class_tcp_server.add<&tcp_server::__destruct>("__destruct");
		class_tcp_server.add<&tcp_server::listen>("listen", {
			php::of_string("addr"),
			php::of_integer("port"),
		});
		class_tcp_server.add<&tcp_server::accept>("accept");
		class_tcp_server.add(php::property_entry("local_addr", nullptr));
		class_tcp_server.add(php::property_entry("local_port", nullptr));
		class_tcp_server.add<&tcp_server::close>("close");
		extension.add(std::move(class_tcp_server));
	}
	tcp_server::tcp_server()
	: listener_(nullptr)
	, binded_(false) {

	}
	tcp_server::~tcp_server() {
		if(listener_ != nullptr) {
			evconnlistener_free(listener_);
		}
	}

	php::value tcp_server::__destruct(php::parameters& params) {
		if(listener_ != nullptr) {
			evconnlistener_disable(listener_);
			evutil_closesocket(evconnlistener_get_fd(listener_));
		}
		binded_ = false;
		return nullptr;
	}

	void tcp_server::error_handler(struct evconnlistener *lis, void *ptr) {
		tcp_server* self = reinterpret_cast<tcp_server*>(ptr);
		int error = EVUTIL_SOCKET_ERROR();
		if(self->cb_.is_empty()) {
			throw php::exception(
				(boost::format("accept failed: %s") % evutil_socket_error_to_string(error)).str(),
				error);
		}else{
			// callback 调用逻辑请参考 tcp_socket 相关说明
			php::callable cb = std::move(self->cb_);
			cb(core::make_exception(
				boost::format("accept failed: %s") % evutil_socket_error_to_string(error),
				error
			));
		}
	}

	php::value tcp_server::listen(php::parameters& params) {
		int len = sizeof(local_addr_);
		zend_string* addr = params[0];
		parse_addr_port(
			addr->val, static_cast<int>(params[1]),
			reinterpret_cast<struct sockaddr*>(&local_addr_), &len
		);
		evutil_socket_t socket_ = create_socket(local_addr_.va.sa_family, SOCK_STREAM, IPPROTO_TCP, true);
		if(::bind(socket_,
			reinterpret_cast<struct sockaddr*>(&local_addr_),
			sizeof(local_addr_)) != 0) {
			throw php::exception(
				(boost::format("bind failed: %s") % strerror(errno)).str(), errno);
		}
		binded_ = true;
		listener_ = evconnlistener_new(core::base, tcp_server::accept_handler, this,
			LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE | LEV_OPT_DISABLED, 1024, socket_);
		evconnlistener_set_error_cb(listener_, tcp_server::error_handler);
		return nullptr;
	}

	php::value tcp_server::local_addr(php::parameters& params) {
		php::string str(64);
		parse_addr(local_addr_.va.sa_family, reinterpret_cast<struct sockaddr*>(&local_addr_), str.data(), str.length());
		return std::move(str);
	}

	php::value tcp_server::local_port(php::parameters& params) {
		return ntohs(local_addr_.v4.sin_port); // v4 和 v6 sin_port 位置重合
	}

	php::value tcp_server::accept(php::parameters& params) {
		if(!binded_) throw php::exception("accept failed: not listened");

		return php::value([this] (php::parameters& params) -> php::value {
			cb_ = params[0];
			evconnlistener_enable(listener_);
			return nullptr;
		});
	}

	void tcp_server::accept_handler(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int size_of_addr, void *ptr) {
		php::object cli_= php::object::create<tcp_socket>();
		tcp_socket* cli = cli_.native<tcp_socket>();
		cli->socket_ = bufferevent_socket_new(core::base, fd, BEV_OPT_CLOSE_ON_FREE);
		bufferevent_setcb(cli->socket_, tcp_socket::read_handler,
			tcp_socket::write_handler, tcp_socket::event_handler, cli);
		cli->connected_ = true;
		// 远端地址的处理
		std::memcpy(&cli->remote_addr_, addr, size_of_addr);

		tcp_server* self = reinterpret_cast<tcp_server*>(ptr);
		// cb 调用方式请参考 tcp_socket 中相关说明
		php::callable cb = std::move(self->cb_);
		cb(nullptr, std::move(cli_));
	}

	php::value tcp_server::close(php::parameters& params) {
		if(listener_ != nullptr) {
			evconnlistener_disable(listener_);
			evutil_closesocket(evconnlistener_get_fd(listener_));
		}
		binded_ = false;
		return nullptr;
	}
}
