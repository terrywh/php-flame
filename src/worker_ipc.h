#pragma once
#include "vendor.h"
#include "ipc.h"

class worker_ipc {
public:
    worker_ipc(boost::asio::io_context& io, std::uint8_t idx);
    virtual ~worker_ipc();
    using socket_ptr = std::shared_ptr<boost::asio::local::stream_protocol::socket>;
    
    virtual std::ostream& output() = 0;
    // 请求并等待响应
    std::shared_ptr<ipc::message_t> ipc_request(std::shared_ptr<ipc::message_t> req, coroutine_handler& ch);
    // 请求（无响应）
    void ipc_request(std::shared_ptr<ipc::message_t> data);
    // 简化 notify 请求
    void ipc_notify(std::uint8_t target, php::value data);
    void ipc_start();
    // 读取及消息监听
    void ipc_run(coroutine_handler ch);
    // 关闭通道
    void ipc_close();
    // IPC 检测
    bool ipc_enabled();
protected:
    virtual std::shared_ptr<worker_ipc> ipc_self() = 0;
    virtual bool on_message(std::shared_ptr<ipc::message_t> msg) = 0;
private:
    boost::asio::io_context&                       io_;
    std::filesystem::path                      svrsck_;
    std::uint8_t                                  idx_;
    socket_ptr                                 socket_;
    std::map<std::uint32_t, ipc::callback_t> callback_;
    std::list<std::shared_ptr<ipc::message_t>>  sendq_;
    int                                        status_ = -1;

    void send(std::shared_ptr<ipc::message_t> msg);
    void send_next();
};
