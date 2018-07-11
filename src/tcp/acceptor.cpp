#include "../coroutine.h"
#include "acceptor.h"
#include "server.h"
#include "socket.h"

namespace flame {
namespace tcp {
	typedef boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT> reuse_port;

	acceptor::acceptor(std::shared_ptr<flame::coroutine> co, server* svr)
	: co_(co)
	, svr_(svr)
	, acceptor_(context)
	, socket_(context) {
		acceptor_.open(svr_->addr_.protocol());
		boost::asio::socket_base::reuse_address opt1(true);
		acceptor_.set_option(opt1);
		reuse_port opt2(true);
		acceptor_.set_option(opt2);
		acceptor_.bind(svr_->addr_);
		acceptor_.listen();
	}
	void acceptor::accept(const boost::system::error_code& error) { BOOST_ASIO_CORO_REENTER(this) {
		co_ = flame::coroutine::current;
		do {
			// 连接等待
			BOOST_ASIO_CORO_YIELD acceptor_.async_accept(socket_,
				std::bind(&acceptor::accept, this->shared_from_this(), std::placeholders::_1));
			if(error == boost::asio::error::operation_aborted) {
				co_->resume();
				return;
			}else if(error) {
				co_->fail(error);
				return;
			}
			// 启动协程执行回调
			BOOST_ASIO_CORO_FORK {
				php::object s(php::class_entry<socket>::entry());
				socket* s_ = static_cast<socket*>(php::native(s));
				s_->socket_ = std::move(socket_);
				s.set("local_address", (boost::format("%s:%d") % s_->socket_.local_endpoint().address().to_string() % s_->socket_.local_endpoint().port()).str());
				s.set("remote_address", (boost::format("%s:%d") % s_->socket_.remote_endpoint().address().to_string() % s_->socket_.remote_endpoint().port()).str());
				std::make_shared<flame::coroutine>()->start(svr_->cb_, {s});
			}
		}while(is_parent());
	}}
}
}