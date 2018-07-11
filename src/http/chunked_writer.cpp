#include "../coroutine.h"
#include "chunked_writer.h"
#include "value_body.h"
#include "server_response.h"
#include "handler.h"
#include "http.h"

namespace flame {
namespace http {
	chunked_writer::chunked_writer(server_response* res, std::shared_ptr<flame::coroutine> co)
	: res_(res)
	, co_(co) {

	}
	// chunked_writer::~chunked_writer() {

	// }
	//!!! 这个运行过程中, 没有进行 response 对象引用的复制(在 handler 中存在)
	void chunked_writer::write(const boost::system::error_code& error, std::size_t n) { BOOST_ASIO_CORO_REENTER(this) {
		if(step_ >= STEP_WRITE_HEADER && !res_->sr_.is_header_done()) {
			// 丢失 yield 符号, 可能会连续调用下面过程, 导致 res_->sr_ 状态异常 (abort)
			if(res_->status_ & server_response::STATUS_HEADER_SENT) {
				co_->fail("header already sent");
				return;
			}
			res_->status_ |= server_response::STATUS_HEADER_SENT;
			res_->build_ex();
			res_->ctr_.chunked(true);
			BOOST_ASIO_CORO_YIELD boost::beast::http::async_write_header(res_->handler_->socket_, res_->sr_,
				std::bind(&chunked_writer::write, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
				
			if(error == boost::asio::error::eof || error == boost::asio::error::broken_pipe) {
				co_->resume();
				return;
			}else if(error) {
				co_->fail(error);
				return;
			}
		}
		if(step_ >= STEP_WRITE_CHUNK && !res_->sr_.is_done()) {
			if(res_->status_ & server_response::STATUS_RESPONSE_END) {
				co_->fail("response already done");
				return;
			}
			if(chunk_.size() > 0) {
				BOOST_ASIO_CORO_YIELD boost::asio::async_write(res_->handler_->socket_,
					boost::beast::http::make_chunk(boost::asio::const_buffer(chunk_.c_str(), chunk_.size())),
					std::bind(&chunked_writer::write, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
				chunk_ = php::string(nullptr);
				if(error == boost::asio::error::eof || error == boost::asio::error::broken_pipe) {
					co_->resume();
					return;
				}else if(error) {
					co_->fail(error);
					return;
				}
			}
		}
		if(step_ >= STEP_WRITE_CHUNK_LAST && !res_->sr_.is_done()) {
			if(res_->status_ & server_response::STATUS_RESPONSE_END) {
				co_->fail("response already done");
				return;
			}
			res_->status_ |= server_response::STATUS_RESPONSE_END;
			BOOST_ASIO_CORO_YIELD boost::asio::async_write(res_->handler_->socket_,
				boost::beast::http::make_chunk_last(),
				std::bind(&chunked_writer::write, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
			if(error == boost::asio::error::eof || error == boost::asio::error::broken_pipe) {
				co_->resume();
				return;
			}else if(error) {
				co_->fail(error);
				return;
			}
		}
		res_->set("body", nullptr);
		co_->resume(); // 继续当前协程 resume 可能会设置新的 step_ 值
	}}
	void chunked_writer::start(int step) {
		step_ = step;
		write();
	}
	void chunked_writer::start(int step, const php::string& chunk) {
		step_ = step;
		chunk_ = ctype_encode(res_->ctr_.find(boost::beast::http::field::content_type)->value(), chunk);
		write();
	}
}
}
