#include "../vendor.h"
#include "udp_socket.h"
#include "../core.h"
#include "init.h"

namespace net {
#define UDP_SOCKET_RBUFFER_SIZE 64 * 1024
	void udp_socket::init(php::extension_entry& extension) {
		php::class_entry<udp_socket> class_udp_socket("flame\\net\\udp_socket");
		class_udp_socket.add<&udp_socket::__destruct>("__destruct");
		class_udp_socket.add<&udp_socket::connect>("connect", {
			php::of_string("addr"),
			php::of_integer("port"),
		});
		// class_udp_server 不提供 connect 方法
		class_udp_socket.add<&udp_socket::remote_addr>("remote_addr");
		class_udp_socket.add<&udp_socket::remote_port>("remote_port");
		class_udp_socket.add<&udp_socket::local_addr>("local_addr");
		class_udp_socket.add<&udp_socket::local_port>("local_port");
		class_udp_socket.add<&udp_socket::read>("read");
		class_udp_socket.add<&udp_socket::write>("write", {
			php::of_string("data"),
		});
		class_udp_socket.add<&udp_socket::write2>("write2", {
			php::of_string("data"),
			php::of_string("addr"),
			php::of_integer("port"),
		});
		class_udp_socket.add<&udp_socket::close>("close");
		extension.add(std::move(class_udp_socket));

		php::class_entry<udp_socket> class_udp_server("flame\\net\\udp_server");
		class_udp_server.add<&udp_socket::__destruct>("__destruct");
		class_udp_server.add<&udp_socket::bind>("bind", {
			php::of_string("addr"),
			php::of_integer("port"),
		});
		// class_udp_server 不提供 connect 方法
		class_udp_server.add<&udp_socket::remote_addr>("remote_addr");
		class_udp_server.add<&udp_socket::remote_port>("remote_port");
		class_udp_server.add<&udp_socket::local_addr>("local_addr");
		class_udp_server.add<&udp_socket::local_port>("local_port");
		class_udp_server.add<&udp_socket::read>("read");
		class_udp_server.add<&udp_socket::write2>("write2", {
			php::of_string("data"),
			php::of_string("addr"),
			php::of_integer("port"),
		});
		class_udp_server.add<&udp_socket::close>("close");
		extension.add(std::move(class_udp_server));
	}

	udp_socket::udp_socket()
	: rbuffer_(nullptr)
	, connected_(false) {

	}
	udp_socket::~udp_socket() {
		if(rbuffer_ != nullptr) {
			zend_string_release(rbuffer_);
		}
	}

	php::value udp_socket::bind(php::parameters& params) {
		int len = sizeof(local_addr_);
		zend_string* addr = params[0];
		parse_addr_port(
			addr->val, static_cast<int>(params[1]),
			reinterpret_cast<struct sockaddr*>(&local_addr_), &len
		);
		socket_ = create_socket(local_addr_.va.sa_family, SOCK_DGRAM, IPPROTO_UDP, true);
		if(::bind(socket_,
			reinterpret_cast<struct sockaddr*>(&local_addr_),
			sizeof(local_addr_)) != 0) {
			throw php::exception(
				(boost::format("bind failed: %s") % strerror(errno)).str(), errno);
		}
		binded_ = true;
		return nullptr;
	}

	php::value udp_socket::connect(php::parameters& params) {
		int rlen = sizeof(remote_addr_);
		zend_string* addr = params[0];
		parse_addr_port(
			addr->val, static_cast<int>(params[1]),
			reinterpret_cast<struct sockaddr*>(&remote_addr_), (int*)&rlen
		);
		socket_ = create_socket(remote_addr_.va.sa_family, SOCK_DGRAM, IPPROTO_UDP, false);
		// TODO 使用 dns_base 解析域名后再进行 “连接”
		if(-1 == ::connect(socket_,
			reinterpret_cast<struct sockaddr*>(&remote_addr_), sizeof(remote_addr_))) {
			throw php::exception(
				(boost::format("bind failed: %s") % strerror(errno)).str(), errno);
		}
		socklen_t llen = sizeof(local_addr_);
		getsockname(socket_, reinterpret_cast<struct sockaddr*>(&local_addr_), &llen);
		connected_ = true;
		return nullptr;
	}

	php::value udp_socket::__destruct(php::parameters& params) {
		evutil_closesocket(socket_);
		return nullptr;
	}

	php::value udp_socket::remote_addr(php::parameters& params) {
		php::string str(64);
		parse_addr(remote_addr_.va.sa_family, reinterpret_cast<struct sockaddr*>(&remote_addr_), str.data(), str.length());
		return std::move(str);
	}

	php::value udp_socket::remote_port(php::parameters& params) {
		return ntohs(remote_addr_.v4.sin_port); // v4 和 v6 sin_port 位置重合
	}

	php::value udp_socket::local_addr(php::parameters& params) {
		php::string str(64);
		parse_addr(local_addr_.va.sa_family, reinterpret_cast<struct sockaddr*>(&local_addr_), str.data(), str.length());
		return std::move(str);
	}

	php::value udp_socket::local_port(php::parameters& params) {
		return ntohs(local_addr_.v4.sin_port); // v4 和 v6 sin_port 位置重合
	}

	php::value udp_socket::read(php::parameters& params) {
		if(!binded_ && !connected_) throw php::exception("read failed: not binded or connected");
		if(rbuffer_ == nullptr) {
			rbuffer_ = zend_string_alloc(UDP_SOCKET_RBUFFER_SIZE, false);
		}
		event_assign(&ev_, core::base, socket_, EV_READ, udp_socket::read_handler, this);
		return php::value([this] (php::parameters& params) -> php::value {
			cb_ = params[0];
			event_add(&ev_, nullptr);
			return nullptr;
		});
	}

	void udp_socket::read_handler(evutil_socket_t fd, short events, void* data) {
		udp_socket* self = reinterpret_cast<udp_socket*>(data);
		if(events & EV_READ) {
			socklen_t len = sizeof(self->remote_addr_);
			if(self->connected_) {
				self->rbuffer_->len = recv( fd, self->rbuffer_->val,
					UDP_SOCKET_RBUFFER_SIZE, 0);
			}else if(self->binded_) {
				self->rbuffer_->len = recvfrom(
					fd, self->rbuffer_->val, UDP_SOCKET_RBUFFER_SIZE,
					0, reinterpret_cast<struct sockaddr*>(&self->remote_addr_), &len);
			}
			php::callable cb = std::move(self->cb_);
			if(self->rbuffer_->len < 0) {
				cb(core::make_exception(
					boost::format("read failed: %s") % strerror(errno),
					errno
				));
			}else if(self->rbuffer_->len == 0) {
				cb(php::make_exception("read failed: EOF", -1));
			}else{
				cb(nullptr, php::value(self->rbuffer_));
			}
		}
	}

	php::value udp_socket::write2(php::parameters& params) {
		if(connected_) throw php::exception("write2 failed: 'write' should be used on connected socket");
		wbuffer_          = params[0];
		zend_string* addr = params[1];
		int          port = params[2];
		int          len  = sizeof(remote_addr_);
		parse_addr_port(addr->val, port,
			reinterpret_cast<struct sockaddr*>(&remote_addr_), &len);

		event_assign(&ev_, core::base, socket_, EV_WRITE, udp_socket::write2handler, this);
		return php::value([this] (php::parameters& params) -> php::value {
			cb_ = params[0];
			event_add(&ev_, nullptr);
			return nullptr;
		});
	}

	php::value udp_socket::write(php::parameters& params) {
		if(!connected_) throw php::exception("write failed: not connected");
		wbuffer_ = params[0];
		event_assign(&ev_, core::base, socket_, EV_WRITE, udp_socket::write_handler, this);
		return php::value([this] (php::parameters& params) -> php::value {
			cb_ = params[0];
			event_add(&ev_, nullptr);
			return nullptr;
		});
	}

	void udp_socket::write_handler(evutil_socket_t socket_, short events, void* data) {
		udp_socket* self = reinterpret_cast<udp_socket*>(data);
		if(self->wbuffer_ == nullptr) {
			php::callable cb = std::move(self->cb_);
			cb(nullptr, true);
		}else{
			ssize_t n = ::send(
				socket_, self->wbuffer_->val, self->wbuffer_->len, MSG_NOSIGNAL);
			if(n > 0) {
				self->wbuffer_ = nullptr;
				event_add(&self->ev_, nullptr); // 等待缓冲数据发送完成
			}else{
				php::callable cb = std::move(self->cb_);
				cb(core::make_exception(
					boost::format("write failed: %s") % strerror(errno),
					errno));
			}
		}
	}

	void udp_socket::write2handler(evutil_socket_t socket_, short events, void* data) {
		udp_socket* self = reinterpret_cast<udp_socket*>(data);
		if(self->wbuffer_ == nullptr) {
			php::callable cb = std::move(self->cb_);
			cb(nullptr, true);
		}else{
			ssize_t n = ::sendto(
				socket_, self->wbuffer_->val, self->wbuffer_->len, MSG_NOSIGNAL,
				reinterpret_cast<struct sockaddr*>(&self->remote_addr_),
				sizeof(self->remote_addr_));

			if(n > 0) {
				self->wbuffer_ = nullptr;
				event_add(&self->ev_, nullptr); // 等待缓冲数据发送完成
			}else{
				php::callable cb = std::move(self->cb_);
				cb(core::make_exception(
					boost::format("write failed: %s") % strerror(errno),
					errno));
			}
		}
	}

	php::value udp_socket::close(php::parameters& params) {
		evutil_closesocket(socket_);
		connected_ = false;
		binded_ = false;
		return nullptr;
	}
}
