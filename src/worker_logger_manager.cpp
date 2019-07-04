#include "worker_logger_manager.h"
#include "worker_logger.h"
#include "worker_ipc.h"

worker_logger_manager::~worker_logger_manager() {
    std::cout << "~worker_logger_manager\n";
}

std::shared_ptr<worker_logger> worker_logger_manager::lm_connect(const std::string& file, coroutine_handler& ch) {
    std::filesystem::path path = file;
    path = path.lexically_normal();

    for (auto i=logger_.begin();i!=logger_.end();) {
        if(i->second.expired()) i = logger_.erase(i);
        else {
            std::shared_ptr<worker_logger> lg = i->second.lock();
            if(lg->path_ == path) return lg;
            ++i;
        }
    }
    
    std::shared_ptr<worker_logger> wl;
    std::uint8_t index = 0;
    if (ipc_->ipc_enabled()) {
        auto msg = ipc::create_message();
        msg->command = ipc::COMMAND_LOGGER_CONNECT;
        msg->unique_id = ipc::create_uid();
        std::memcpy(msg->payload, file.data(), file.size());
        msg = ipc_->ipc_request(msg, ch);
        index = msg->target;
        // 使用该日志文件标号创建 LOGGER 对象，以支持实际日志发送
        wl = std::make_shared<worker_logger>(this, file, msg->target);
    }
    else {
        index = lindex_;
        wl = std::make_shared<worker_logger>(this, file, index, true);
        ++lindex_;
    }
    
    logger_.emplace(index, wl);
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
    msg->length  = 0;
    ipc_->ipc_request(msg);
}

void worker_logger_manager::lm_close() {

}