#include "../dynamic_buffer.h"
#include "../coroutine.h"
#include "socket.h"

namespace flame {
namespace tcp {
	void socket::declare(php::extension_entry& ext) {
		php::class_entry<socket> class_socket("flame\\tcp\\socket");
		class_socket
			.property({"local_address", ""})
			.property({"remote_address", ""})
			.method<&socket::read>("read", {
				{"complete", php::TYPE::UNDEFINED, false, true}
			})
			.method<&socket::write>("write", {
				{"data", php::TYPE::STRING}
			})
			.method<&socket::close>("close");
		ext.add(std::move(class_socket));
	}
	socket::socket()
	: socket_(context) {

	}
	php::value socket::read(php::parameters& params) {
		std::shared_ptr<coroutine> co = coroutine::current;
		php::object ref(this);
		if(params.size() == 0) { // 随意读取一段数据
			socket_.async_read_some(boost::asio::buffer(buffer_.prepare(2048), 2048), [this, co, ref] (const boost::system::error_code& error, std::size_t n) {
				if(error == boost::asio::error::operation_aborted || error == boost::asio::error::eof) {
					co->resume();
				}else if(error) {
					co->fail(error);
				}else{
					buffer_.commit(n);
					php::string data(buffer_.data(), buffer_.size());
					buffer_.consume(buffer_.size());
					co->resume(data);
				}
			});
		}else if(params[0].typeof(php::TYPE::STRING)) { // 读取到指定的结束符
			php::string delim = params[0];
			boost::asio::async_read_until<boost::asio::ip::tcp::socket, dynamic_buffer>(socket_, buffer_, delim.to_string(),
				[this, co, ref] (const boost::system::error_code& error, std::size_t n) {

				if(error == boost::asio::error::operation_aborted || error == boost::asio::error::eof) {
					co->resume();
				}else if(error) {
					co->fail(error);
				}else{
					php::string data(buffer_.data(), n);
					buffer_.consume(n);
					co->resume(data);
				}
			});
		}else if(params[0].typeof(php::TYPE::INTEGER)) { // 读取指定长度
			std::size_t want = params[0].to_integer();
			if(buffer_.size() >= want) {
				php::string r(buffer_.data(), want);
				buffer_.consume(want);
				return r;
			}
			boost::asio::async_read<boost::asio::ip::tcp::socket, dynamic_buffer>(socket_, buffer_,
				boost::asio::transfer_exactly(params[0].to_integer() - buffer_.size()), // 剩余的缓存数据也算在长度中
				[this, co, ref] (const boost::system::error_code& error, std::size_t n) {

				if(error == boost::asio::error::operation_aborted || error == boost::asio::error::eof) {
					co->resume();
				}else if(error) {
					co->fail(error);
				}else{
					php::string data(buffer_.data(), buffer_.size());
					buffer_.consume(buffer_.size());
					co->resume(data);
				}
			});
		}
		
		return coroutine::async();
	}
	php::value socket::write(php::parameters& params) {
		php::string str = params[0];
		str.to_string();
		if(q_.empty()) {
			q_.push_back({coroutine::current, str});
			write_ex();
		}else{
			q_.push_back({coroutine::current, str});
		}
		return coroutine::async();
	}
	void socket::write_ex() {
		php::object ref(this);
		boost::asio::async_write(socket_, boost::asio::buffer(q_.front().second.c_str(), q_.front().second.size()), [this, ref] (const boost::system::error_code& error, std::size_t n) {
			std::shared_ptr<coroutine> co = q_.front().first;
			q_.pop_front();
			if(!q_.empty()) {
				write_ex();
			}
			// 下面 resume 等使用可能再次触发 write 动作
			if(error == boost::asio::error::operation_aborted) {
				co->resume();
			}else if(error) {
				co->fail(error);
			}else{
				co->resume();
			}
		});
	}
	php::value socket::close(php::parameters& params) {
		socket_.close();
		return nullptr;
	}
}
}
  