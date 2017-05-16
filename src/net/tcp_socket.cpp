#include "../vendor.h"
#include "tcp_socket.h"
#include "../core.h"
#include "init.h"

namespace net {
#define TCP_MAX_BUFFER_SIZE 8 * 1024 * 1024
	void tcp_socket::init(php::extension_entry& extension) {
		php::class_entry<tcp_socket> class_tcp_socket("flame\\net\\tcp_socket");
		class_tcp_socket.add<&tcp_socket::__construct>("__construct");
		class_tcp_socket.add<&tcp_socket::__destruct>("__destruct");
		class_tcp_socket.add<&tcp_socket::connect>("connect", {
			php::of_string("addr"),
			php::of_integer("port"),
		});
		class_tcp_socket.add<&tcp_socket::remote_addr>("remote_addr");
		class_tcp_socket.add<&tcp_socket::remote_port>("remote_port");
		class_tcp_socket.add<&tcp_socket::local_addr>("local_addr");
		class_tcp_socket.add<&tcp_socket::local_port>("local_port");
		class_tcp_socket.add<&tcp_socket::read>("read", {
			php::of_mixed("condition"),
		});
		class_tcp_socket.add<&tcp_socket::write>("write", {
			php::of_string("data"),
		});
		class_tcp_socket.add<&tcp_socket::close>("close");
		extension.add(std::move(class_tcp_socket));
	}
	tcp_socket::tcp_socket(bool connected)
	: socket_(nullptr)
	, connected_(connected)
	, length_(0) {

	}

	php::value tcp_socket::__construct(php::parameters& params) {
		// TODO 超时时间选项及实现
		return nullptr;
	}

	php::value tcp_socket::__destruct(php::parameters& params) {
		bufferevent_free(socket_);
		return nullptr;
	}

	php::value tcp_socket::connect(php::parameters& params) {
		zend_string* host = params[0];
		int   port = params[1];
		socket_ = bufferevent_socket_new(core::base, -1, BEV_OPT_CLOSE_ON_FREE);
		bufferevent_setcb(socket_, tcp_socket::read_handler,
			tcp_socket::write_handler, tcp_socket::event_handler, this);

		return php::value([this, host, port] (php::parameters& params) -> php::value {
			cb_ = params[0];
			if(bufferevent_socket_connect_hostname(socket_, core::base_dns, AF_UNSPEC, host->val, port) == -1) {
				php::callable cb = std::move(cb_);
				cb(core::make_exception(
					boost::format("connect failed: %s") % strerror(errno),
					errno));
			}
			return nullptr;
		});
	}

	void tcp_socket::event_handler(struct bufferevent* bev, short events, void *ctx) {
		tcp_socket* self = reinterpret_cast<tcp_socket*>(ctx);
		if(events & BEV_EVENT_ERROR) {
			// 执行 cb 会 resume 当前 generator，若无新 cb_ 设置，
			// 如果不释放 cb_ 相当于内存泄漏
			php::callable cb = std::move(self->cb_);
			int error = EVUTIL_SOCKET_ERROR();
			cb(core::make_exception(
				boost::format("%s failed: %s") % (events & BEV_EVENT_READING ? "read" : "write") % evutil_socket_error_to_string(error),
				error
			));
			// 直接使用 cb_ 对象，无法在此释放：由于可能产生新的 cb_ 设置，使用 cb_.reset() 会释放错误对象
		}else if(events & BEV_EVENT_TIMEOUT) {
			php::callable cb = std::move(self->cb_);
			cb(core::make_exception(
				boost::format("%s failed: ETIMEDOUT") % (events & BEV_EVENT_READING ? "read" : "write"),
				ETIMEDOUT
			));
		}else if(events & BEV_EVENT_EOF) {
			php::callable cb = std::move(self->cb_);
			cb(core::make_exception(
				boost::format("%s failed: EOF") % (events & BEV_EVENT_READING ? "read" : "write"),
				-1
			));
		}else if(events & BEV_EVENT_CONNECTED) {
			self->connected_ = true;
			// 连接后 初始化 本地地址
			socklen_t llen = sizeof(self->local_addr_);
			getsockname(bufferevent_getfd(self->socket_), reinterpret_cast<struct sockaddr*>(&self->local_addr_), &llen);
			php::callable cb = std::move(self->cb_);
			cb(nullptr, true);
		}else{
			assert(0);
		}
	}

	php::value tcp_socket::remote_addr(php::parameters& params) {
		php::string str(64);
		parse_addr(remote_addr_.va.sa_family, reinterpret_cast<struct sockaddr*>(&local_addr_), str.data(), str.length());
		return std::move(str);
	}

	php::value tcp_socket::remote_port(php::parameters& params) {
		return ntohs(remote_addr_.v4.sin_port); // v4 和 v6 sin_port 位置重合
	}

	php::value tcp_socket::local_addr(php::parameters& params) {
		php::string str(64);
		parse_addr(local_addr_.va.sa_family, reinterpret_cast<struct sockaddr*>(&local_addr_), str.data(), str.length());
		return std::move(str);
	}

	php::value tcp_socket::local_port(php::parameters& params) {
		return ntohs(local_addr_.v4.sin_port); // v4 和 v6 sin_port 位置重合
	}

	php::value tcp_socket::read(php::parameters& params) {
		if(!connected_) throw php::exception("read failed: not connected");
		// 提供三种方式的数据读取机制：
		if(params.length() == 0) { // 1. 读多少算多少
			length_ = 0;
		}else if(params[0].is_long()) { // 2. 读取指定长度
			length_ = params[0];
		}else{ // 3. 读取到指定分隔符
			length_ = -1;
			delim_  = params[0];
		}
		return php::value([this] (php::parameters& params) -> php::value {
			cb_ = params[0];
			bufferevent_enable(socket_, EV_READ);
			return nullptr;
		});
	}
	void tcp_socket::read_handler(struct bufferevent *socket_, void *ctx) {
		tcp_socket* self = reinterpret_cast<tcp_socket*>(ctx);
		if(self->length_ > 0) { // 按长度读取方式
			size_t length = evbuffer_get_length(bufferevent_get_input(socket_));
			if(length >= self->length_) {
				php::string data(self->length_);
				bufferevent_read(socket_, data.data(), self->length_);
				// !!!! 注意 disable 调用必须在 cb_ 调用之前
				bufferevent_disable(socket_, EV_READ);
				php::callable cb = std::move(self->cb_);
				cb(nullptr, std::move(data));
			}else if(length > TCP_MAX_BUFFER_SIZE) {
				// 当读取到的数据已经足够多，还未找到结束符时，抛出错误
				// !!!! 注意 disable 调用必须在 cb_ 调用之前
				bufferevent_disable(socket_, EV_READ);
				php::callable cb = std::move(self->cb_);
				cb(php::make_exception("read failed: data overflow", EOVERFLOW));
			}else{
				return;
			}
		}else if(self->length_ == 0) { // 有多少读多少
			size_t length = evbuffer_get_length(bufferevent_get_input(socket_));
			php::string data(length);
			bufferevent_read(socket_, data.data(), length);
			// !!!! 注意 disable 调用必须在 cb_ 调用之前
			bufferevent_disable(socket_, EV_READ);
			php::callable cb = std::move(self->cb_);
			cb(nullptr, std::move(data));
		}else/* if(self->length_ < 0)*/ { // 按结束符读取方式
			// TODO 优化搜索方式：记录上次搜索位置
			evbuffer_ptr p = evbuffer_search(
				bufferevent_get_input(socket_),
				self->delim_->val, self->delim_->len,
				nullptr);
			if(p.pos >= 0) {
				php::string data(p.pos + self->delim_->len);
				bufferevent_read(socket_, data.data(), data.length());
				// !!!! 注意 disable 调用必须在 cb_ 调用之前
				bufferevent_disable(socket_, EV_READ);
				php::callable cb = std::move(self->cb_);
				cb(nullptr, std::move(data));
			}else if(evbuffer_get_length(bufferevent_get_input(socket_)) > TCP_MAX_BUFFER_SIZE) {
				// 当读取到的数据已经足够多，还未找到结束符时，抛出错误
				// !!!! 注意 disable 调用必须在 cb_ 调用之前
				bufferevent_disable(socket_, EV_READ);
				php::callable cb = std::move(self->cb_);
				cb(php::make_exception("read failed: data overflow", EOVERFLOW));
			}else{
				return;
			}
		}
	}
	php::value tcp_socket::write(php::parameters& params) {
		if(!connected_) throw php::exception("write failed: not connected");
		wbuffer_ = params[0];
		return php::value([this] (php::parameters& params) -> php::value {
			cb_ = params[0];
			bufferevent_enable(socket_, EV_WRITE);
			return nullptr;
		});
	}
	void tcp_socket::write_handler(struct bufferevent *socket_, void *ctx) {
		tcp_socket* self = reinterpret_cast<tcp_socket*>(ctx);
		// 发送后再次进入 write_handler 标识：发送完成
		if(self->wbuffer_ == nullptr) {
			// !!!! 注意 disable 调用必须在 cb_ 调用之前
			bufferevent_disable(socket_, EV_WRITE);
			php::callable cb = std::move(self->cb_);
			cb(nullptr, true);
		}else if(-1 == bufferevent_write(socket_, self->wbuffer_->val, self->wbuffer_->len)) {
			// !!!! 注意 disable 调用必须在 cb_ 调用之前
			bufferevent_disable(socket_, EV_WRITE);
			php::callable cb = std::move(self->cb_);
			cb(core::make_exception(
				boost::format("write failed: %s") % strerror(errno),
				errno
			));
		}else{
			self->wbuffer_ = nullptr;
			return;
		}
	}
	php::value tcp_socket::close(php::parameters& params) {
		bufferevent_free(socket_);
		connected_ = false;
		return nullptr;
	}
}
