// !!! included in executor.h

namespace flame {
namespace http {
    template <class Stream>
    executor<Stream>::executor(const php::object& cli, const php::object& req)
    : cli_ref(cli)
    , req_ref(req)
    , res_ref(nullptr)
    , timer_(context) {
        cli_ = static_cast<client*>(php::native(cli_ref));
        req_ = static_cast<client_request*>(php::native(req_ref));
    }
    template <class Stream>
    void executor<Stream>::execute(const boost::system::error_code& error, std::size_t n) { BOOST_ASIO_CORO_REENTER(this) {
        co_ = flame::coroutine::current;
        // 构建并发送请求
        req_->build_ex();
        BOOST_ASIO_CORO_YIELD cli_->acquire(req_->url_, s_, std::bind(&executor::execute, this->shared_from_this(), std::placeholders::_1, 0));
        if(error == boost::asio::error::operation_aborted) {
            co_->fail("request timeout", boost::system::errc::stream_timeout);
            return;
        }else if(error) {
            co_->fail(error);
            return;
        }
        timer_.expires_after(std::chrono::milliseconds(static_cast<int>(req_->get("timeout"))));
        timer_.async_wait(std::bind(&executor::timeout, this->shared_from_this(), std::placeholders::_1));

        BOOST_ASIO_CORO_YIELD boost::beast::http::async_write(*s_, req_->ctr_, std::bind(&executor::execute, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        if(error == boost::asio::error::operation_aborted) {
            co_->fail("request timeout", boost::system::errc::stream_timeout);
            return;
        }else if(error) {
            co_->fail(error);
            return;
        }
        // 构建并读取响应
        res_ref = php::object(php::class_entry<client_response>::entry());
        res_    = static_cast<client_response*>(php::native(res_ref));
        BOOST_ASIO_CORO_YIELD boost::beast::http::async_read(*s_, cache_, res_->ctr_, std::bind(&executor::execute, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        if(error == boost::asio::error::operation_aborted) {
            co_->fail("request timeout", boost::system::errc::stream_timeout);
            return;
        }else if(error) {
            co_->fail(error);
            return;
        }
        res_->build_ex();
        timer_.cancel();
        // 返回响应
        co_->resume(res_ref);
    } }
    template <class Stream>
    void executor<Stream>::timeout(const boost::system::error_code& error) {
        if(!error) {
            s_->lowest_layer().cancel();
        }
    }
}
}