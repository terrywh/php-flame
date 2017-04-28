#include "../vendor.h"
#include "addr.h"
#include "tcp_socket.h"

namespace net {
	php::value tcp_socket::__construct(php::parameters& params) {
		std::int64_t dead = -1;
		if(params.length() > 2) {
			dead = mill_now() + static_cast<std::int64_t>(params[3]);
		}
		remote_addr_ = mill_ipremote(params[0], params[1], 0, dead);
		if(errno != 0) {
			throw php::exception("failed to connect (dns)", errno);
		}
		std::printf("tcp_socket::__construct\n");
		socket_ = mill_tcpconnect(remote_addr_, dead);
		if(errno != 0) {
			throw php::exception("failed to connect (tcp)", errno);
		}
		closed_ = false;
		prop("closed", closed_);
		return nullptr;
	}
	php::value tcp_socket::__destruct(php::parameters& params) {
		if(!closed_) {
			closed_ = true;
			std::printf("before close: %08x %08x\n", this, socket_);
			mill_tcpclose(socket_);
			std::printf("after close: %08x %08x\n", this, socket_);
		}
		return nullptr;
	}
	php::value tcp_socket::remote_addr(php::parameters& params) {
		php::value addr = php::value::object<net::addr_t>();
		addr.native<net::addr_t>()->addr_ = remote_addr_;
		addr.native<net::addr_t>()->port_ = mill_ipport(remote_addr_);
		return std::move(addr);
	}
	php::value tcp_socket::recv(php::parameters& params) {
		php::value   stop = params[0];
		std::int64_t dead = -1;
		if(params.length() > 1) {
			dead = mill_now() + static_cast<std::int64_t>(params[1]);
		}
		if(stop.is_long()) { // 接收一定量
			return recv_length(stop, dead);
		}else if(stop.is_string()) { // 接收到指定任意停止符号
			return recv_delims(stop, dead);
		}
		throw php::exception("unsupported recv condition: long for specified length or string for specified delim expected");
	}
	php::value tcp_socket::recv_length(int length, std::int64_t dead) {
		php::value   r = php::value::string(length);
		zend_string* b = r;
		b->len = mill_tcprecv(socket_, b->val, length, dead);
		if(errno != 0) {
			throw php::exception("failed to recv length", errno);
		}
		return std::move(r);
	}
	php::value tcp_socket::recv_delims(zend_string* delims, std::int64_t dead) {
		php::buffer b(1024); // 初始容量
AGAIN:
		char* x = b.rev(512);
		std::size_t n = mill_tcprecvuntil(socket_, x, 512, delims->val, delims->len, dead);
		b.put(n);
		if(errno == ENOBUFS) { // 未遇到结束符
			goto AGAIN;
		}else if(errno != 0) {
			throw php::exception("failed to recv delim", errno);
		}
		return std::move(b);
	}
	php::value tcp_socket::send_buffer(php::parameters& params) {
		zend_string*  b = params[0];
		std::int64_t  d = -1;
		if(params.length() > 1) {
			d = mill_now() + static_cast<std::int64_t>(params[1]);
		}
		std::size_t n = mill_tcpsend(socket_, b->val, b->len, d);
		if(errno != 0) {
			throw php::exception("failed to send_buffer", errno);
		}
		return php::value((std::int64_t)n);
	}
	php::value tcp_socket::flush(php::parameters& params) {
		std::int64_t d = -1;
		if(params.length() > 1) {
			d = mill_now() + static_cast<std::int64_t>(params[1]);
		}
		mill_tcpflush(socket_, d);
		if(errno != 0) {
			throw php::exception("failed to flush", 0);
		}
		return nullptr;
	}
	php::value tcp_socket::send(php::parameters& params) {
		std::printf("socket::send: %08x %08x\n", this, socket_);
		zend_string* b = params[0];
		std::int64_t d = -1;
		if(params.length() > 1) {
			d = mill_now() + static_cast<std::int64_t>(params[1]);
		}
		std::size_t n = mill_tcpsend(socket_, b->val, b->len, d);
		if(errno != 0) {
			throw php::exception("failed to send (send)", errno);
		}
		mill_tcpflush(socket_, d);
		if(errno != 0) {
			throw php::exception("failed to send (flush)", errno);
		}
		return php::value((std::int64_t)n);
	}
	php::value tcp_socket::close(php::parameters& params) {
		if(!closed_) {
			closed_ = true;
			prop("closed", closed_);
			std::printf("before close: %08x %08x\n", this, socket_);
			mill_tcpclose(socket_);
			std::printf("after close: %08x %08x\n", this, socket_);
		}
		return nullptr;
	}
}
