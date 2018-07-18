#include "../coroutine.h"
#include "chunked_writer.h"
#include "handler.h"
#include "value_body.h"

namespace flame {
namespace http {
	chunked_writer::chunked_writer(std::shared_ptr<flame::coroutine> co, std::shared_ptr<handler> h, int target, int status)
	: co_(co)
	, handler_(h)
	, target_(target)
	, status_(status)
	, sr_(new boost::beast::http::serializer<false, value_body<false>>(*handler_->res_)) {

	}
	chunked_writer::chunked_writer(std::shared_ptr<flame::coroutine> co, std::shared_ptr<handler> h, int target, const php::string& chunk, int status)
	: co_(co)
	, handler_(h)
	, target_(target)
	, status_(status)
	, sr_(new boost::beast::http::serializer<false, value_body<false>>(*handler_->res_))
	, chunk_(chunk) {

	}
	// chunked_writer::~chunked_writer() {

	// }
	//!!! 这个运行过程中, 没有进行 response 对象引用的复制(在 handler 中存在)
	void chunked_writer::write(const boost::system::error_code& error, std::size_t n) { BOOST_ASIO_CORO_REENTER(this) {

		if(!error && target_ >= RESPONSE_TARGET_WRITE_HEADER && !(status_ & RESPONSE_STATUS_HEADER_SENT)) {
			BOOST_ASIO_CORO_YIELD boost::beast::http::async_write_header(handler_->socket_, *sr_,
				std::bind(&chunked_writer::write, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}
		if(!error && target_ >= RESPONSE_TARGET_WRITE_CHUNK && !(status_ & RESPONSE_STATUS_FINISHED)) {
			if(chunk_.size() > 0) {
				BOOST_ASIO_CORO_YIELD boost::asio::async_write(handler_->socket_,
					boost::beast::http::make_chunk(boost::asio::const_buffer(chunk_.c_str(), chunk_.size())),
					std::bind(&chunked_writer::write, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
				chunk_ = php::string(nullptr);
			}
		}
		if(!error && target_ >= RESPONSE_TARGET_WRITE_CHUNK_LAST && !(status_ & RESPONSE_STATUS_FINISHED)) {
			BOOST_ASIO_CORO_YIELD boost::asio::async_write(handler_->socket_, boost::beast::http::make_chunk_last(),
				std::bind(&chunked_writer::write, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}
		// 本次操作结束
		if(!error || error == boost::asio::error::eof || error == boost::asio::error::broken_pipe || error == boost::asio::error::connection_reset) {
			co_->resume();
		}else if(error) {
			co_->fail(error);
		}
		// 脱离了 Http 回调协程时, 需要恢复 HANDLER 复用连接
		if((status_ & RESPONSE_STATUS_DETACHED) && target_ >= RESPONSE_TARGET_WRITE_CHUNK_LAST) handler_->handle(error, n);
	}}
	void chunked_writer::start() {
		write(boost::system::error_code(), 0);
	}
}
}
