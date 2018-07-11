#include "../coroutine.h"
#include "socket.h"

namespace flame {
namespace udp {
	void socket::declare(php::extension_entry& ext) {
		php::class_entry<socket> class_socket("flame\\udp\\socket");
		class_socket
			.property({"local_address", ""})
			.property({"remote_address", ""})
			.method<&socket::__construct>("__construct", {
				{"bind", php::TYPE::STRING, false, true},
			})
			.method<&socket::receive>("receive")
			.method<&socket::receive_from>("receive_from", {
				{"from", php::TYPE::UNDEFINED, true},
			})
			.method<&socket::send>("send", {
				{"data", php::TYPE::STRING}
			})
			.method<&socket::send_to>("send_to", {
				{"data", php::TYPE::STRING},
				{"to", php::TYPE::STRING},
			})
			.method<&socket::close>("close");
		ext.add(std::move(class_socket));
	}
	socket::socket()
	: socket_(context) {

	}
	typedef boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT> reuse_port;
	php::value socket::__construct(php::parameters& params) {
		if(params.size() > 0) {
			php::string str = params[0];
			char *s = str.data(), *p, *e = s + str.size();
			for(p = s; p < e; ++p) {
				// 分离 地址与端口
				if(*p == ':') break;
			}
			if(*p != ':') throw php::exception(zend_ce_exception, "udp socket __construct failed: address port missing");
			boost::asio::ip::udp::endpoint addr(boost::asio::ip::make_address(std::string(s, p - s)), std::atoi(p+1));

			socket_.open(addr.protocol());

			boost::asio::socket_base::reuse_address opt1(true);
			socket_.set_option(opt1);
			reuse_port opt2(true);
			socket_.set_option(opt2);

			socket_.bind(addr);
		}
		return nullptr;
	}
	php::value socket::receive(php::parameters& param) {
		php::object ref(this);
		std::shared_ptr<coroutine> co = coroutine::current;
		socket_.async_receive(boost::asio::buffer(buffer_.prepare(64 * 1024), 64 * 1024), [this, co, ref] (const boost::system::error_code& error, std::size_t n) {
			if(error) return co->fail(error);
			// buffer_.commit(n);
			// TODO 若接收到的数据较大, 改用 move 的方式?
			co->resume(php::string(buffer_.data(), n)); // 复制 (非移动, 避免重复申请缓冲区 64K 内存)
		});
		return coroutine::async();
	}
	php::value socket::receive_from(php::parameters& params) {
		php::string from = params[0];
		std::shared_ptr<coroutine> co = coroutine::current;
		std::shared_ptr<boost::asio::ip::udp::endpoint> edp = std::make_shared<boost::asio::ip::udp::endpoint>();
		php::object ref(this);
		socket_.async_receive_from(boost::asio::buffer(buffer_.prepare(64 * 1024), 64 * 1024), *edp, [this, co, edp, from, ref] (const boost::system::error_code& error, std::size_t n) {
			if(error) return co->fail(error);
			// buffer_.commit(n);
			php::string f = from;
			f = (boost::format("%s:%d") % edp->address().to_string() % edp->port()).str();
			// TODO 若接收到的数据较大, 改用 move 的方式?
			co->resume(php::string(buffer_.data(), n)); // 复制 (非移动, 避免重复申请缓冲区 64K 内存)
		});
		return coroutine::async();
	}
	php::value socket::send(php::parameters& params) {
		php::string data = params[0];
		php::object ref(this);
		std::shared_ptr<coroutine> co = coroutine::current;
		socket_.async_send(boost::asio::buffer(data.c_str(), data.size()), [this, co, data, ref] (const boost::system::error_code& error, std::size_t n) {
			if(error) return co->fail(error);
			co->resume();
		});
		return coroutine::async();
	}
	php::value socket::send_to(php::parameters& params) {
		php::string str = params[1];
		char *s = str.data(), *p, *e = s + str.size();
		for(p = s; p < e; ++p) {
			// 分离 地址与端口
			if(*p == ':') break;
		}
		if(*p != ':') throw php::exception(zend_ce_exception, "udp socket send failed: address port missing");
		boost::asio::ip::udp::endpoint addr(boost::asio::ip::make_address(std::string(s, p - s)), std::atoi(p+1));

		php::string data = params[0];
		php::object ref(this);
		std::shared_ptr<coroutine> co = coroutine::current;
		socket_.async_send_to(boost::asio::buffer(data.c_str(), data.size()), addr, [this, co, data, ref] (const boost::system::error_code& error, std::size_t n) {
			if(error) return co->fail(error);
			co->resume();
		});
		return coroutine::async();
	}
	php::value socket::close(php::parameters& params) {
		socket_.close();
		return nullptr;
	}
}
}
