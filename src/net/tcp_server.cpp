#include "../vendor.h"
#include "tcp_server.h"
#include "../core.h"
#include "init.h"
#include "tcp_socket.h"

namespace net {

	void tcp_server::init(php::extension_entry& extension) {
		php::class_entry<tcp_server> ce_tcp_server("flame\\net\\tcp_server");
		ce_tcp_server.add<&tcp_server::__construct>("__construct");
		ce_tcp_server.add<&tcp_server::__destruct>("__destruct");
		ce_tcp_server.add<&tcp_server::listen>("listen", {
			php::of_string("addr"),
			php::of_integer("port"),
		});
		ce_tcp_server.add<&tcp_server::accept>("accept");
		ce_tcp_server.add(php::property_entry("local_addr", nullptr));
		ce_tcp_server.add(php::property_entry("local_port", nullptr));
		ce_tcp_server.add<&tcp_server::close>("close");
		extension.add(std::move(ce_tcp_server));
	}
	tcp_server::tcp_server()
	: acceptor_(core::io())
	, is_ipv6_(true) // 默认按 IPv6
	, listened_(false) {

	}
	php::value tcp_server::__construct(php::parameters& params) {
		boost::system::error_code err;
		acceptor_.open(tcp::v6(), err); // 优先使用 v6 协议（能够兼容 v4）
		if(err) {
			is_ipv6_ = false;
			acceptor_.open(tcp::v4(), err);
		}
		if(err) {
			throw php::exception("failed to create: " + err.message(), err.value());
		}
		return nullptr;
	}
	php::value tcp_server::listen(php::parameters& params) {
		boost::system::error_code err;
		std::string addr = params[0].is_null() ? "" : params[0];
		int port = params[1];
#ifdef SO_REUSEPORT
		// 服务端需要启用下面选项，以支持更高性能的多进程形式
		int opt = 1;
		if(0 != setsockopt(acceptor_.native_handle(), SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof (opt))) {
			throw php::exception("failed to bind (SO_REUSEPORT)", errno);
		}
#endif
		acceptor_.bind(tcp::endpoint(addr_from_str(addr, is_ipv6_), port), err);
		if(err) {
			throw php::exception("failed to bind: " + err.message(), err.value());
		}
		acceptor_.listen();
		set_prop_local_addr();
		listened_ = true;
		return nullptr;
	}
	void tcp_server::set_prop_local_addr() {
		auto ep = acceptor_.local_endpoint();
		prop("local_addr") = ep.address().to_string();
		prop("local_port") = ep.port();
	}
	php::value tcp_server::__destruct(php::parameters& params) {
		boost::system::error_code err;
		acceptor_.close(err); // 存在重复关闭的可能，排除错误
		return nullptr;
	}
	php::value tcp_server::accept(php::parameters& params) {
		if(!listened_) throw php::exception("accept failed: not listened");
		return php::value([this] (php::parameters& params) -> php::value {
			php::callable done = params[0];

			php::object cli = php::object::create<tcp_socket>();
			tcp_socket* cli_= cli.native<tcp_socket>();
			acceptor_.async_accept(cli_->socket_, cli_->remote_,
				[this, done, cli, cli_] (const boost::system::error_code& err) mutable {
					if(err) {
						done(core::error_to_exception(err));
					}else{
						cli_->connected_ = true;
						done(nullptr, cli);
					}
				});
			return nullptr;
		});
	}
	php::value tcp_server::close(php::parameters& params) {
		boost::system::error_code err;
		acceptor_.close(err); // 存在重复关闭的可能，排除错误
		listened_ = false;
		return nullptr;
	}
}
