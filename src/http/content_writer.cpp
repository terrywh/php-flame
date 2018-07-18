#include "../coroutine.h"
#include "content_writer.h"
#include "handler.h"
#include "value_body.h"

namespace flame {
namespace http {
	content_writer::content_writer(std::shared_ptr<flame::coroutine> co, std::shared_ptr<handler> h, php::string body, int status)
	: co_(co)
    , handler_(h)
    , body_(body) // 保证 body 的生存期
    , status_(status) {

	}
	// content_writer::~content_writer() {

	// }
	void content_writer::start() {
		write(boost::system::error_code(), 0);
	}
	//!!! 这个运行过程中, 没有进行 response 对象引用的复制(在 handler 中存在)
	void content_writer::write(const boost::system::error_code& error, std::size_t n) { BOOST_ASIO_CORO_REENTER(this) {

        BOOST_ASIO_CORO_YIELD boost::beast::http::async_write(handler_->socket_, *handler_->res_,
            std::bind(&content_writer::write, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));

		if(co_) {
	        if(error == boost::asio::error::broken_pipe || error == boost::asio::error::connection_reset || !error) {
	            co_->resume();
	        }else{
	            co_->fail(error);
	        }
		}
		// 脱离了 Http 回调协程时, 需要恢复 HANDLER 复用连接
		if(status_ & RESPONSE_STATUS_DETACHED) handler_->handle(error, n);
	}}
}
}
