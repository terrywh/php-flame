#include "../vendor.h"
#include "tcp_socket.h"
#include "../core.h"
#include "init.h"

namespace net {

	void tcp_socket::init(php::extension_entry& extension) {
		php::class_entry<tcp_socket> ce_tcp_socket("flame\\net\\tcp_socket");
		ce_tcp_socket.add<&tcp_socket::__construct>("__construct");
		ce_tcp_socket.add<&tcp_socket::__destruct>("__destruct");
		ce_tcp_socket.add<&tcp_socket::connect>("connect", {
			php::of_string("addr"),
			php::of_integer("port"),
		});
		ce_tcp_socket.add<&tcp_socket::remote_addr>("remote_addr");
		ce_tcp_socket.add<&tcp_socket::remote_port>("remote_port");
		ce_tcp_socket.add<&tcp_socket::read>("read");
		ce_tcp_socket.add<&tcp_socket::write>("write", {
			php::of_string("data"),
		});
		ce_tcp_socket.add(php::property_entry("local_addr", nullptr));
		ce_tcp_socket.add(php::property_entry("local_port", nullptr));
		ce_tcp_socket.add<&tcp_socket::close>("close");
		extension.add(std::move(ce_tcp_socket));
	}
	tcp_socket::tcp_socket(bool connected)
	: socket_(core::io())
	, buffer_(8 * 1024 * 1024) // TODO buffer_ 最大尺寸？
	, connected_(connected)
	, is_ipv6_(true) { // 默认按 IPv6

	}
	php::value tcp_socket::__construct(php::parameters& params) {
		boost::system::error_code err;
		socket_.open(tcp::v6(), err); // 优先使用 v6 协议（一般来说能够兼容 v4）
		if(err) {
			is_ipv6_ = false;
			socket_.open(tcp::v4(), err);
		}
		if(err) {
			throw php::exception("failed to create: " + err.message(), err.value());
		}
		return nullptr;
	}
	php::value tcp_socket::connect(php::parameters& params) {
		boost::system::error_code err;
		std::string addr = params[0];
		int port = params[1];
		remote_ = tcp::endpoint(addr_from_str(addr, is_ipv6_), port);
		return php::value([this] (php::parameters& params) -> php::value {
			php::callable& done = params[0];
			socket_.async_connect(remote_, [this, done] (const boost::system::error_code& err) mutable {
				if(err) {
					done(core::error_to_exception(err));
				}else{
					connected_ = true;
					set_prop_local_addr();
					done(nullptr);
				}
			});
			return nullptr;
		});
	}
	void tcp_socket::set_prop_local_addr() {
		auto ep = socket_.local_endpoint();
		prop("local_addr") = ep.address().to_string();
		prop("local_port") = ep.port();
	}
	php::value tcp_socket::__destruct(php::parameters& params) {
		boost::system::error_code err;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, err);
		socket_.close(err); // 存在重复关闭的可能，排除错误
		return nullptr;
	}
	php::value tcp_socket::remote_addr(php::parameters& params) {
		return remote_.address().to_string();
	}
	php::value tcp_socket::remote_port(php::parameters& params) {
		return remote_.port();
	}
	php::value tcp_socket::read(php::parameters& params) {
		if(!connected_) throw php::exception("read failed: not connected");
		// 考虑提供两种方式的数据读取机制：
		if(params[0].is_long()) { // 1. 读取指定长度
			std::size_t length = params[0];
			return php::value([this, length] (php::parameters& params) -> php::value {
				auto buf = buffer_.prepare(length);
				php::callable done = params[0];
				boost::asio::async_read(socket_, buf, [this, done] (const boost::system::error_code& err, std::size_t n) mutable {
					buffer_.commit(n);
					if(err) {
						done(core::error_to_exception(err));
					}else{
						php::string buf(n);
						buffer_.sgetn(buf.data(), n);
						done(nullptr, buf);
					}
				});
				return nullptr;
			});
		}else if(params[0].is_string()) { // 2. 读取到指定分隔符
			std::string delim = params[0];
			return php::value([this, delim] (php::parameters& params) -> php::value {
				php::callable& done = params[0];
				boost::asio::async_read_until(socket_, buffer_, delim, [this, done] (const boost::system::error_code& err, std::size_t n) mutable {
					if(err) {
						done(core::error_to_exception(err));
					}else{
						php::string buf(n);
						buffer_.sgetn(buf.data(), n);
						done(nullptr, buf);
					}
				});
				return nullptr;
			});
		}else{
			throw php::exception("failed to read: unknown read method");
		}
	}
	php::value tcp_socket::write(php::parameters& params) {
		if(!connected_) throw php::exception("write failed: not connected");
		zend_string* data = params[0];
		return php::value([this, data] (php::parameters& params) -> php::value {
			php::callable& done = params[0];
			boost::asio::async_write(
				socket_,
				// 需要进行类型转换，否则 asio::buffer 会将 zend_string -> val 长度解为 1
				boost::asio::buffer(reinterpret_cast<char*>(data->val), data->len),
				[this, done] (const boost::system::error_code& err, std::size_t n) mutable {
					if(err) {
						done(core::error_to_exception(err));
					}else{
						done(nullptr, static_cast<std::int64_t>(n));
					}
				});
			return nullptr;
		});
	}
	php::value tcp_socket::close(php::parameters& params) {
		boost::system::error_code err;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, err);
		socket_.close(err); // 存在重复关闭的可能，排除错误
		connected_ = false;
		return nullptr;
	}
}
