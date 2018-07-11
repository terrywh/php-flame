#include "../coroutine.h"
#include "acceptor.h"
#include "server.h"
#include "handler.h"

namespace flame {
namespace http {
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
		do {
			BOOST_ASIO_CORO_YIELD acceptor_.async_accept(socket_,
				std::bind(&acceptor::accept, this->shared_from_this(), std::placeholders::_1));
			if(error == boost::asio::error::operation_aborted) {
				co_->resume();
				return;
			}else if(error) {
				co_->fail(error);
				return;
			}
			BOOST_ASIO_CORO_FORK std::make_shared<handler>(svr_, std::move(socket_), co_)->handle();
		}while(is_parent());
	}}
}
}