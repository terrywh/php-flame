#include "../controller.h"
#include "../coroutine.h"
#include "socket.h"

namespace flame::tcp {

	void socket::declare(php::extension_entry& ext) {
		php::class_entry<socket> class_socket("flame\\tcp\\socket");
		class_socket
			.property({"local_address", ""})
			.property({"remote_address", ""})
			.method<&socket::read>("read", {
				{"completion", php::TYPE::UNDEFINED, false, true}
			})
			.method<&socket::write>("write", {
				{"data", php::TYPE::STRING}
			})
			.method<&socket::close>("close");
		ext.add(std::move(class_socket));
	}

	socket::socket()
	: socket_(gcontroller->context_x) {

	}
    
	php::value socket::read(php::parameters& params) {
        coroutine_handler ch{coroutine::current};
        // 使用下面锁保证不会同时进行读取
        coroutine_guard guard(rmutex_, ch);

		boost::system::error_code err;
        std::size_t len;
        if (params.size() == 0) {// 1. 随意读取一段数据
            len = socket_.async_read_some(boost::asio::buffer(buffer_.prepare(8192), 8192), ch[err]);
            buffer_.commit(len);
        }
        else if (params[0].typeof(php::TYPE::STRING)) { // 2. 读取到指定的结束符
			std::string delim = params[0];
            len = boost::asio::async_read_until(socket_, buffer_, delim, ch[err]);
        }
        else if (params[0].typeof(php::TYPE::INTEGER)) { // 3. 读取指定长度
			std::size_t want = params[0].to_integer();
			if(buffer_.size() >= want) {
                len = want;
                goto RETURN_DATA;
			}
            // 读取指定长度 (剩余的缓存数据也算在长度中)
            len = boost::asio::async_read(socket_, buffer_, boost::asio::transfer_exactly(want - buffer_.size()), ch[err]);
            len = want;
		}
        else throw php::exception(zend_ce_type_error
            , "Failed to read TCP socket: unknown completion type"
            , -1); // 未知读取方式
RETURN_DATA:
        // 数据返回
        if (err == boost::asio::error::operation_aborted || err == boost::asio::error::eof) return nullptr;
        else if (err) throw php::exception(zend_ce_exception
            , (boost::format("Failed to read TCP socket: %s") % err.message()).str()
            , err.value());
        else {
            php::string data(len);
            boost::asio::buffer_copy(boost::asio::buffer(data.data(), len), buffer_.data(), len);
            buffer_.consume(len);
            return std::move(data);
        }
    }

	php::value socket::write(php::parameters& params) {
        coroutine_handler ch{coroutine::current};
        // 使用下面锁保证不会同时写入
        coroutine_guard guard(wmutex_, ch);
        
        boost::system::error_code err;
		std::string data = params[0];
        boost::asio::async_write(socket_, boost::asio::buffer(data), ch);

        if (!err || err == boost::asio::error::operation_aborted) return nullptr;
        else throw php::exception(zend_ce_exception
            , (boost::format("Failed to write TCP socket: %s") % err.message()).str()
            , err.value());
	}

	php::value socket::close(php::parameters& params) {
		socket_.shutdown(boost::asio::socket_base::shutdown_both);
		return nullptr;
	}
}
  