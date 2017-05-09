#include "../vendor.h"
#include "udp_socket.h"
#include "../core.h"
#include "init.h"

namespace net {

	void udp_socket::init(php::extension_entry& extension) {
		php::class_entry<udp_socket> ce_udp_socket("flame\\net\\udp_socket");
		php::class_entry<udp_socket> ce_udp_server("flame\\net\\udp_server");
		ce_udp_socket.add<&udp_socket::__construct>("__construct");
		ce_udp_server.add<&udp_socket::__construct>("__construct");
		ce_udp_socket.add<&udp_socket::__destruct>("__destruct");
		ce_udp_server.add<&udp_socket::__destruct>("__destruct");
		// ce_udp_socket 不提供 bind 方法
		ce_udp_server.add<&udp_socket::bind>("bind", {
			php::of_string("addr"),
			php::of_integer("port"),
		});
		ce_udp_socket.add<&udp_socket::connect>("connect", {
			php::of_string("addr"),
			php::of_integer("port"),
		});
		// ce_udp_server 不提供 connect 方法
		ce_udp_socket.add<&udp_socket::remote_addr>("remote_addr");
		ce_udp_server.add<&udp_socket::remote_addr>("remote_addr");
		ce_udp_socket.add<&udp_socket::remote_port>("remote_port");
		ce_udp_server.add<&udp_socket::remote_port>("remote_port");
		ce_udp_socket.add<&udp_socket::read>("read");
		ce_udp_server.add<&udp_socket::read>("read");
		ce_udp_socket.add<&udp_socket::write>("write", {
			php::of_string("data"),
		});
		// ce_udp_server 不提供 write 方法（仅提供 write_to）
		ce_udp_socket.add<&udp_socket::write_to>("write_to", {
			php::of_string("data"),
			php::of_string("addr"),
			php::of_integer("port"),
		});
		ce_udp_server.add<&udp_socket::write_to>("write_to", {
			php::of_string("data"),
			php::of_string("addr"),
			php::of_integer("port"),
		});
		ce_udp_socket.add(php::property_entry("local_addr", nullptr));
		ce_udp_server.add(php::property_entry("local_addr", nullptr));
		ce_udp_socket.add(php::property_entry("local_port", nullptr));
		ce_udp_server.add(php::property_entry("local_port", nullptr));
		ce_udp_socket.add<&udp_socket::close>("close");
		ce_udp_server.add<&udp_socket::close>("close");
		extension.add(std::move(ce_udp_socket));
		extension.add(std::move(ce_udp_server));
	}

	udp_socket::udp_socket()
	: socket_(core::io())
	, connected_(false)
	, is_ipv6_(true) { // 默认按 IPv6

	}
	php::value udp_socket::__construct(php::parameters& params) {
		boost::system::error_code err;
		socket_.open(udp::v6(), err); // 优先使用 v6 协议（一般来说能够兼容 v4）
		if(err) {
			is_ipv6_ = false;
			socket_.open(udp::v4(), err);
		}
		if(err) {
			throw php::exception("failed to create: " + err.message(), err.value());
		}
		return nullptr;
	}

	php::value udp_socket::bind(php::parameters& params) {
		boost::system::error_code err;
		std::string addr = params[0].is_null() ? "" : params[0];
		int port = params[1];
#ifdef SO_REUSEPORT
		// 服务端需要启用下面选项，以支持更高性能的多进程形式
		int opt = 1;
		if(0 != setsockopt(socket_.native_handle(), SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof (opt))) {
			throw php::exception("failed to bind (SO_REUSEPORT)", errno);
		}
#endif
		socket_.bind(udp::endpoint(addr_from_str(addr, is_ipv6_), port), err);
		if(err) {
			throw php::exception("failed to bind: " + err.message(), err.value());
		}
		set_prop_local_addr();
		connected_ = true;
		return nullptr;
	}
	php::value udp_socket::connect(php::parameters& params) {
		boost::system::error_code err;
		std::string addr = params[0];
		int port = params[1];
		remote_ = udp::endpoint(addr_from_str(addr, is_ipv6_), port);
		return php::value([this] (php::parameters& params) -> php::value {
			php::callable done = params[0];
			socket_.async_connect(remote_, [this, done] (const boost::system::error_code& err) mutable {
				if(err) {
					done(core::error_to_exception(err));
				}else{
					connected_ = true;
					set_prop_local_addr();
					done(nullptr);
				}
			});
		});
	}
	void udp_socket::set_prop_local_addr() {
		auto ep = socket_.local_endpoint();
		prop("local_addr") = ep.address().to_string();
		prop("local_port") = ep.port();
	}
	php::value udp_socket::__destruct(php::parameters& params) {
		boost::system::error_code err;
		socket_.close(err); // 存在重复关闭的可能，排除错误
		return nullptr;
	}
	php::value udp_socket::remote_addr(php::parameters& params) {
		return remote_.address().to_string();
	}
	php::value udp_socket::remote_port(php::parameters& params) {
		return remote_.port();
	}
	php::value udp_socket::read(php::parameters& params) {
		if(connected_) throw php::exception("read failed: bind or connect required");
		return php::value([this] (php::parameters& params) -> php::value {
			php::callable done = params[0];
			socket_.async_receive_from(
				boost::asio::buffer(buffer_), remote_,
				[this, done] (const boost::system::error_code& err, std::size_t n) mutable {
					if(err) {
						done(core::error_to_exception(err));
					}else{
						done(nullptr, php::value(buffer_, n));
					}
				});
			return nullptr;
		});
	}
	php::value udp_socket::write_to(php::parameters& params) {
		if(connected_) throw php::exception("write_to failed: use 'write' instead");
		zend_string* data = params[0];
		zend_string* addr = params[1];
		int          port = params[2];
		remote_.address(address::from_string(std::string(addr->val, addr->len)));
		remote_.port(port);
		return php::value([this, data] (php::parameters& params) -> php::value {
			php::callable done = params[0];
			// 需要进行类型转换，否则 asio::buffer 会将 zend_string -> val 长度解为 1
			socket_.async_send_to(
				boost::asio::buffer(reinterpret_cast<char*>(data->val), data->len),
				remote_, [this, done] (const boost::system::error_code& err, std::size_t n) mutable {
					if(err) {
						done(core::error_to_exception(err));
					}else{
						done(nullptr, static_cast<std::int64_t>(n));
					}
				});
			return nullptr;
		});
	}
	php::value udp_socket::write(php::parameters& params) {
		if(!connected_) throw php::exception("write failed: not connected");
		zend_string* data = params[0];
		return php::value([this, data] (php::parameters& params) -> php::value {
			php::callable done = params[0];
			// asio::buffer 会将 zend_string -> val 长度解为 1
			socket_.async_send(
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
	php::value udp_socket::close(php::parameters& params) {
		boost::system::error_code err;
		socket_.close(err); // 存在重复关闭的可能，排除错误
		connected_ = false;
		return nullptr;
	}
}
