#pragma once
#include "vendor.h"
#include "coroutine.h"

class ipc {
public:
    // 传输协议：消息格式
    struct message_t {
        std::uint8_t  command;
        std::uint8_t  target;
        std::uint16_t length;
        std::uint32_t unique_id;
        char payload[0];
    };
    // 传输协议：命令
    enum command_t {
        MESSAGE_INIT_CAPACITY = 2048,

        COMMAND_REGISTER       = 0x01,
        COMMAND_LOGGER_CONNECT = 0x02,
        COMMAND_LOGGER_DESTROY = 0x03,
        COMMAND_LOGGER_DATA    = 0x04,

        COMMAND_TRANSFER_TO_CHILD = 0x10,
        COMMAND_NOTIFY_DATA    = 0x10,
    };
    struct callback_t {
        coroutine_handler&               cch;
        std::shared_ptr<ipc::message_t>& res;
    };
    // 消息容器构建
    static message_t* malloc_message(std::uint16_t length = ipc::MESSAGE_INIT_CAPACITY - sizeof(ipc::message_t));
    // 消息容器释放
    static void free_message(message_t* msg);
    // 消息容器构建
    static std::shared_ptr<message_t> create_message();
    // 消息容器长度调整
    static void relloc_message(std::shared_ptr<ipc::message_t>& msg, std::uint16_t copy, std::uint16_t length);
    // 生成消息ID
    static std::uint32_t create_uid();
    // 请求并等待响应   
    virtual std::shared_ptr<ipc::message_t> ipc_request(std::shared_ptr<ipc::message_t> data, coroutine_handler& ch) = 0;
    // 请求（无响应）
    virtual void ipc_request(std::shared_ptr<ipc::message_t> data) = 0;
};
