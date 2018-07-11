#include "../coroutine.h"
#include "file_writer.h"
#include "value_body.h"
#include "server_response.h"
#include "handler.h"
#include "http.h"

namespace flame {
namespace http {
	file_writer::file_writer(server_response* res, std::shared_ptr<flame::coroutine> co, boost::filesystem::path path)
	: res_(res)
	, co_(co)
	, path_(path)
	, fd_(0)
	, ds_(context) {
		boost::system::error_code error;
		if(boost::filesystem::is_regular_file(path_, error)) {
			fd_ = ::open(path_.native().c_str(), O_RDONLY);
			ds_.assign(fd_);
		}
	}
	// file_writer::~file_writer() {
		
	// }
	//!!! 这个运行过程中, 没有进行 response 对象引用的复制(在 handler 中存在)
	void file_writer::write(const boost::system::error_code& error, std::size_t n) { BOOST_ASIO_CORO_REENTER(this) {
		// 丢失 yield 符号, 可能会连续调用下面过程, 导致 res_->sr_ 状态异常 (abort)
		if(res_->status_ & server_response::STATUS_RESPONSE_END) {
			co_->fail("response already done");
			return;
		}
		if(!fd_) {
			if((res_->status_ & server_response::STATUS_HEADER_SENT) == 0) {
				res_->set("status", 404);
				res_->build_ex();
			}
			res_->status_ |= server_response::STATUS_HEADER_SENT | server_response::STATUS_RESPONSE_END;
			res_->ctr_.body() = php::string( "file '" + path_.filename().native() + "' not found" );
			BOOST_ASIO_CORO_YIELD boost::beast::http::async_write(res_->handler_->socket_, res_->sr_,
				std::bind(&file_writer::write, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}else{
			if((res_->status_ & server_response::STATUS_HEADER_SENT) == 0) {
				res_->ctr_.chunked(true); // 使用 chunked 方式发送文件	
				res_->build_ex();
			}
			res_->status_ |= server_response::STATUS_HEADER_SENT | server_response::STATUS_RESPONSE_END;
			BOOST_ASIO_CORO_YIELD boost::beast::http::async_write_header(res_->handler_->socket_, res_->sr_,
				std::bind(&file_writer::write, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}
		if(error) {
			co_->fail(error);
			return;
		}
		if(!fd_) return;

		while(true) {
			BOOST_ASIO_CORO_YIELD boost::asio::async_read(ds_, boost::asio::buffer(buffer_),
					std::bind(&file_writer::write, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
			if(n > 0) {
				BOOST_ASIO_CORO_YIELD boost::asio::async_write(res_->handler_->socket_,
					boost::beast::http::make_chunk(boost::asio::const_buffer(buffer_, n)),
					std::bind(&file_writer::write, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
				if(error) {
					co_->fail(error);
					return;
				}
			}else{
				break;
			}
		}
		BOOST_ASIO_CORO_YIELD boost::asio::async_write(res_->handler_->socket_,
			boost::beast::http::make_chunk_last(),
			std::bind(&file_writer::write, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		if(error) {
			co_->fail(error);
			return;
		}
		co_->resume(); // 继续当前协程 resume
		res_->handler_->handle(); // 返回 handle() 将导致当前 res 对象被释放(如果没有用户引用的话)
	}}
	void file_writer::start() {
		write();
	}
}
}