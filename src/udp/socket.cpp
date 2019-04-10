#include "../controller.h"
#include "../coroutine.h"
#include "udp.h"
#include "socket.h"

namespace flame::udp {

	void socket::declare(php::extension_entry& ext)  {
		php::class_entry<socket> class_socket("flame\\udp\\socket");
		class_socket
			.property({"local_address", ""})
			.property({"remote_address", ""})
			.method<&socket::__construct>("__construct", {
				{"address", php::TYPE::STRING, false, true},
			})
			.method<&socket::recv_from>("recv_from", {
				{"address", php::TYPE::STRING, true, true},
			})
			.method<&socket::recv>("recv")
			.method<&socket::send_to>("send_to", {
				{"data", php::TYPE::STRING, false, false},
				{"address", php::TYPE::STRING, false, false},
			})
			.method<&socket::send>("send")
			.method<&socket::close>("close");
		ext.add(std::move(class_socket));
	}

	socket::socket()
	: socket_(gcontroller->context_x)
	, connected_(false)
	, max_(64 * 1024) {

	}

	typedef boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT> reuse_port;

	php::value socket::__construct(php::parameters& params) {
		boost::system::error_code err;
		php::array option;
		if (params.size() > 0 && params[0].typeof(php::TYPE::STRING)) {
			auto pair = addr2pair(params[0]);
			boost::asio::ip::address address = boost::asio::ip::make_address(std::string_view(pair.first.data(), pair.first.size()), err);
			if(err) throw php::exception(zend_ce_exception
	            , (boost::format("Failed to bind: %s") % err.message()).str()
	            , err.value());
			std::uint16_t port = std::stoi(pair.second);
			auto ep = boost::asio::ip::udp::endpoint(address, port);
			socket_.open(ep.protocol());
			boost::asio::socket_base::reuse_address opt1(true);
	        socket_.set_option(opt1);
	        reuse_port opt2(true);
	        socket_.set_option(opt2);
			socket_.bind(ep, err);
			if(err) throw php::exception(zend_ce_exception
	            , (boost::format("failed to bind: %s") % err.message()).str()
	            , err.value());
			if(params.size() > 1 && params[1].typeof(php::TYPE::ARRAY)) {
				option = params[0];
				goto PARSE_OPTION;
			}
		} else {
			socket_.open(boost::asio::ip::udp::v6(), err);
			if (err) socket_.open(boost::asio::ip::udp::v4(), err);
			if (err) throw php::exception(zend_ce_exception
				, (boost::format("Failed to bind: %s") % err.message()).str()
				, err.value());
			if(params.size() > 0 && params[0].typeof(php::TYPE::ARRAY)) {
				option = params[0];
				goto PARSE_OPTION;
			}
		}
		return nullptr;
PARSE_OPTION:
		if(option.exists("max"))
			max_ = std::min(std::max(static_cast<int>(option.get("max")), 64), 64 * 1024);
		return nullptr;
	}

	php::value socket::recv(php::parameters& params) {
		if(!connected_) throw php::exception(zend_ce_error_exception
			, "Failed to send: socket not connected"
			, -1);
        coroutine_handler ch{coroutine::current};
        // 使用下面锁保证不会同时进行读取
        coroutine_guard guard(rmutex_, ch);
		boost::system::error_code err;
        std::size_t len = 0;
        len = socket_.async_receive(boost::asio::buffer(buffer_.prepare(max_), max_), ch[err]);
		buffer_.commit(len);
        // 数据返回
        if (err == boost::asio::error::operation_aborted || err == boost::asio::error::eof) return nullptr;
        else if (err) throw php::exception(zend_ce_exception
            , (boost::format("Failed to recv UDP packet: %s") % err.message()).str()
            , err.value());
        else return php::string(std::move(buffer_));
    }

	php::value socket::recv_from(php::parameters& params) {
        coroutine_handler ch{coroutine::current};
        // 使用下面锁保证不会同时进行读取
        coroutine_guard guard(rmutex_, ch);

		boost::system::error_code err;
		std::size_t len = 0;
		boost::asio::ip::udp::endpoint ep;
		len = socket_.async_receive_from(boost::asio::buffer(buffer_.prepare(max_), max_), ep, ch[err]);
		buffer_.commit(len);
        // 数据返回
        if (err == boost::asio::error::operation_aborted || err == boost::asio::error::eof) return nullptr;
        else if (err) throw php::exception(zend_ce_exception
            , (boost::format("failed to read TCP socket: %s") % err.message()).str()
            , err.value());
		if(params.size() > 0) params[0] = (boost::format("%s:%d") % ep.address().to_string(err) % ep.port()).str();
		return php::string(std::move(buffer_));
    }

	php::value socket::send(php::parameters& params) {
		if(!connected_) throw php::exception(zend_ce_error_exception
			, "Failed to send: connected socket required"
			, -1);
        coroutine_handler ch{coroutine::current};

        boost::system::error_code err;
		std::string data = params[0];
        socket_.async_send(boost::asio::buffer(data), ch[err]);

        if (!err || err == boost::asio::error::operation_aborted) return nullptr;
        else throw php::exception(zend_ce_exception
            , (boost::format("Failed to write TCP socket: %s") % err.message()).str()
            , err.value());
        return nullptr;
	}

	php::value socket::send_to(php::parameters& params) {
        coroutine_handler ch{coroutine::current};

        boost::system::error_code err;
		php::string data = params[0];
		auto pair = addr2pair(params[1]);
		if(pair.first.empty() || pair.second.empty()) throw php::exception(zend_ce_type_error
			, "Failed to send udp packet: illegal address format"
			, -1);

		boost::asio::ip::udp::resolver::results_type eps;
		resolver_->async_resolve(pair.first, pair.second, 
			[&ch, &eps, &err] (const boost::system::error_code& error, boost::asio::ip::udp::resolver::results_type results) {

			if(error) err = error;
			else eps = results;
			ch.resume();
		});
		ch.suspend();
		if (err == boost::asio::error::operation_aborted) return nullptr;
		else if(err) throw php::exception(zend_ce_exception
			, (boost::format("Failed to resolve address: %s") % err.message()).str()
			, err.value());

		// 发送
		int sent = 0;
		for(auto i=eps.begin(); i!=eps.end(); ++i) {
			socket_.async_send_to(boost::asio::buffer(data.c_str(), data.size()), *i, ch[err]);
			if(!err) return nullptr;
		}

		throw php::exception(zend_ce_exception
			, (boost::format("Failed to send UDP packet: %s") % err.message()).str()
			, err.value());
	}
	
	php::value socket::close(php::parameters& params) {
		connected_ = false;
		socket_.shutdown(boost::asio::socket_base::shutdown_both);
		return nullptr;
	}
}
