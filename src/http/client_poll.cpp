#include "client_poll.h"
#include "http.h"
#include "client.h"

namespace flame::http {
    
    client_poll* client_poll::create_poll(boost::asio::io_context &io, curl_socket_t fd, client_poll::poll_callback_t cb, void* data) {
        int       type, r;
        socklen_t size = sizeof(type);
        struct sockaddr addr;
        r = ::getsockopt(fd, SOL_SOCKET, SO_TYPE, reinterpret_cast<char*>(&type), &size);
        assert(r == 0);
        size = sizeof(addr);
        r = ::getsockname(fd, &addr, &size);
        assert(r == 0);
        client_poll* poll;
        if (type == SOCK_STREAM) poll = new client_poll_tcp(io, addr.sa_family == AF_INET6
            ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4(), ::dup(fd));
        else if (type == SOCK_DGRAM) poll = new client_poll_udp(io, addr.sa_family == AF_INET6
            ? boost::asio::ip::udp::v6() : boost::asio::ip::udp::v4(), ::dup(fd));
        else return nullptr;
        poll->data = data;
        poll->fd_ = fd;
        poll->cb_ = cb;
        return poll;
    }

    client_poll::client_poll()
    : action_(CURL_POLL_NONE) {
    }

    client_poll::~client_poll() {
    }

    void client_poll::on_ready(const boost::system::error_code& error, int action) {
        if (error == boost::asio::error::operation_aborted) return; // 取消监听（销毁）或设置了新的监听
        cb_(error, this, fd_, action);
        if (action_ == CURL_POLL_REMOVE) delete this;
        else if (action_ == action) async_wait();
    }

    void client_poll::async_wait(int action) {
        if (action_ != action) {
            action_ = action;
            async_wait();
        }
    }

    client_poll_tcp::client_poll_tcp(boost::asio::io_context &io, boost::asio::ip::tcp proto, curl_socket_t fd)
    : sock_(io, proto, fd) {
    }

    client_poll_tcp::~client_poll_tcp() {
        // sock_.release();
    }

    void client_poll_tcp::async_wait() {
        sock_.cancel();
        // 重新监听
        if (action_ == CURL_POLL_IN || action_ == CURL_POLL_INOUT) 
            sock_.async_wait(boost::asio::ip::tcp::socket::wait_read, std::bind(&client_poll_tcp::on_ready, this, std::placeholders::_1, action_));
        if (action_ == CURL_POLL_OUT || action_ == CURL_POLL_INOUT) 
            sock_.async_wait(boost::asio::ip::tcp::socket::wait_write, std::bind(&client_poll_tcp::on_ready, this, std::placeholders::_1, action_));
    }
    
    client_poll_udp::client_poll_udp(boost::asio::io_context &io, boost::asio::ip::udp proto, curl_socket_t fd)
    : client_poll(), sock_(io, proto, fd) {
    }

    client_poll_udp::~client_poll_udp() {
    }

    void client_poll_udp::async_wait() {
        sock_.cancel();
        // 重新监听
        if (action_ == CURL_POLL_IN || action_ == CURL_POLL_INOUT) 
            sock_.async_wait(boost::asio::ip::tcp::socket::wait_read, std::bind(&client_poll_udp::on_ready, this, std::placeholders::_1, action_));
        if (action_ == CURL_POLL_OUT || action_ == CURL_POLL_INOUT) 
            sock_.async_wait(boost::asio::ip::tcp::socket::wait_write, std::bind(&client_poll_udp::on_ready, this, std::placeholders::_1, action_));
    }

}