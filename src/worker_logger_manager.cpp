#include "worker_logger_manager.h"
#include "worker_logger.h"
#include "worker_ipc.h"
#include "worker_logger_buffer.h"

worker_logger_manager::~worker_logger_manager() {
    std::cout << "~worker_logger_manager\n";
}

std::shared_ptr<worker_logger> worker_logger_manager::lm_connect(const std::string& filepath, coroutine_handler& ch) {
    std::shared_ptr<worker_logger> wl;
    auto i = logger_.find(0);
    if(i == logger_.end() || i->second.expired()) {
        // IPC: 在父进程创建或链接到指定的日志文件
        auto msg = ipc::create_message();
        msg->command = ipc::COMMAND_LOGGER_CONNECT;
        msg->unique_id = ipc::create_uid();
        std::memcpy(msg->payload, filepath.data(), filepath.size());
        // TODO
        wl = std::make_shared<worker_logger>(this, 0);
        /*
        msg = ipc_->ipc_request(msg, ch);
        // 使用该日志文件标号创建 LOGGER 对象，以支持实际日志发送
        wl = std::make_shared<worker_logger>(this, msg->target);
        */
        logger_.emplace(0, wl);
    }
    else wl = i->second.lock();
    return wl;
}

void worker_logger_manager::lm_destroy(std::uint8_t idx) {
    auto i = logger_.find(idx);
    if(i == logger_.end()) return;
    logger_.erase(i);

    // IPC: 通知父进程日志器不再使用
    auto msg = ipc::create_message();
    msg->command = ipc::COMMAND_LOGGER_DESTROY;
    msg->target  = idx;
    // msg->length  = 0;
    ipc_->ipc_request(msg);
}

void worker_logger_manager::lm_close() {
    
}