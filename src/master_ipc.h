#pragma once
#include "vendor.h"
#include "coroutine.h"
#include "ipc.h"

class master_ipc {
public:
    using socket_ptr = std::shared_ptr<boost::asio::local::stream_protocol::socket>;
    master_ipc(boost::asio::io_context& io);
    virtual ~master_ipc();
    // 输出
    virtual std::ostream& output() = 0;
    // 监听连接及连接数据接收
    void ipc_run(coroutine_handler ch);
    // void ipc_read(socket_ptr sock, coroutine_handler ch);
    void ipc_read(socket_ptr sock, coroutine_handler& ch);
    void ipc_close();
    // 请求并等待响应
    std::shared_ptr<ipc::message_t> ipc_request(std::shared_ptr<ipc::message_t> req, coroutine_handler& ch);
    // 请求（无响应）
    void ipc_request(std::shared_ptr<ipc::message_t> data);
protected:
    virtual bool on_message(std::shared_ptr<ipc::message_t> msg, socket_ptr sock);
    virtual std::shared_ptr<master_ipc> ipc_self() = 0;
private:
    boost::asio::io_context&                          io_;
    std::filesystem::path                         svrsck_;
    boost::asio::local::stream_protocol::acceptor server_;
    std::map<std::uint8_t, socket_ptr>            socket_;
    std::map<std::uint32_t, ipc::callback_t>    callback_;
    std::list<std::shared_ptr<ipc::message_t>>     sendq_;
    // 连接数据接收
    
    void send(std::shared_ptr<ipc::message_t> msg);
    void send_next();
};
