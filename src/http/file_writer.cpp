#include "../coroutine.h"
#include "file_writer.h"
#include "handler.h"
#include "value_body.h"

namespace flame {
namespace http {
	file_writer::file_writer(std::shared_ptr<flame::coroutine> co, std::shared_ptr<handler> h, boost::filesystem::path path, int status)
	: co_(co)
	, handler_(h)
	, path_(path)
	, status_(status)
	, fd_(0)
	, ds_(context)
	, sr_(new boost::beast::http::serializer<false, value_body<false>>(*handler_->res_)) {
		boost::system::error_code error;
		if(boost::filesystem::is_regular_file(path_, error)) {
			fd_ = ::open(path_.native().c_str(), O_RDONLY);
			ds_.assign(fd_);
		}
	}
	// file_writer::~file_writer() {

	// }
	//!!! 这个运行过程中, 没有进行 response 对象引用的复制(在 handler 中存在)
	void file_writer::start() {
		if(status_ & RESPONSE_STATUS_FINISHED) {
			co_->fail("response already done");
			return;
		}
		if(!fd_) {
			write_not_found(boost::system::error_code(), 0);
		}else{
			write_file_data(boost::system::error_code(), 0);
		}
	}

	void file_writer::write_not_found(const boost::system::error_code& error, std::size_t n) { BOOST_ASIO_CORO_REENTER(this) {
		handler_->res_->result(boost::beast::http::status::not_found);
		// 头
		BOOST_ASIO_CORO_YIELD boost::beast::http::async_write_header(handler_->socket_, *sr_,
			std::bind(&file_writer::write_not_found, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));

		if(error == boost::asio::error::broken_pipe || error == boost::asio::error::connection_reset) {
			co_->resume();
			return;
		}else if(error){
			co_->fail(error);
			return;
		}
		// 结束
		BOOST_ASIO_CORO_YIELD boost::asio::async_write(handler_->socket_,
			boost::beast::http::make_chunk_last(),
			std::bind(&file_writer::write_not_found, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));

		if(error == boost::asio::error::broken_pipe || error == boost::asio::error::connection_reset || !error) {
			co_->resume();
		}else{
			co_->fail(error);
		}
	}}

	void file_writer::write_file_data(const boost::system::error_code& error, std::size_t n) { BOOST_ASIO_CORO_REENTER(this) {
		handler_->res_->result(boost::beast::http::status::ok);
		// 头
		BOOST_ASIO_CORO_YIELD boost::beast::http::async_write_header(handler_->socket_, *sr_,
			std::bind(&file_writer::write_file_data, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));

		// 文件内容逐块发送
		while(!error) {
			BOOST_ASIO_CORO_YIELD boost::asio::async_read(ds_, boost::asio::buffer(buffer_),
					std::bind(&file_writer::write_file_data, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
			if(n > 0) {
				BOOST_ASIO_CORO_YIELD boost::asio::async_write(handler_->socket_,
					boost::beast::http::make_chunk(boost::asio::const_buffer(buffer_, n)),
					std::bind(&file_writer::write_file_data, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
			}else{
				break;
			}
		}
		// 结束
		if(!error) {
			BOOST_ASIO_CORO_YIELD boost::asio::async_write(handler_->socket_,
				boost::beast::http::make_chunk_last(),
				std::bind(&file_writer::write_not_found, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}
		if(error == boost::asio::error::broken_pipe || error == boost::asio::error::connection_reset) {
			co_->resume();
		}else if(error){
			co_->fail(error);
		}
		// 脱离了 Http 回调协程时, 需要恢复 HANDLER 复用连接
		if(status_ & RESPONSE_STATUS_DETACHED) handler_->handle(error, n);
	}}
}
}
