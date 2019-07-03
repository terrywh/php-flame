#include "worker_logger_buffer.h"
#include "worker_logger_manager.h"
#include "worker_ipc.h"

worker_logger_buffer::worker_logger_buffer(worker_logger_manager* mgr)
: mgr_(mgr)
, msg_(ipc::create_message()) {
    cap_ = msg_->length;
    msg_->length = 0;
}

int worker_logger_buffer::overflow(int ch) {
    char c = ch;
    msg_->payload[msg_->length] = c;
    ++msg_->length;
    if(c == '\n') transfer_msg();
    return ch;
}

long worker_logger_buffer::xsputn(const char* s, long c) {
    if(cap_ - msg_->length < c) {
        assert(cap_ < 2048 * 1024);
        ipc::relloc_message(msg_, msg_->length, msg_->length + c);
    }
    std::memcpy(msg_->payload + msg_->length, s, c);
    msg_->length += c;

    if(s[c-1] == '\n') transfer_msg();
    return c;
}

void worker_logger_buffer::transfer_msg() {
    msg_->command = ipc::COMMAND_NOTIFY_DATA;
    msg_->target  = 0;
    mgr_->ipc_->ipc_request(msg_);
    msg_ = ipc::create_message();
    cap_ = msg_->length;
    msg_->length  = 0; // 以 length 累计当前需要发送的数据
}