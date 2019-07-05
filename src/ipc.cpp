#include "ipc.h"

static boost::pool<> pool_(ipc::MESSAGE_INIT_CAPACITY);

static void free_message(ipc::message_t* msg) {
    pool_.free(msg);
}

std::shared_ptr<ipc::message_t> ipc::create_message(std::uint16_t length) {
    if(length > ipc::MESSAGE_INIT_CAPACITY - sizeof(ipc::message_t)) {
        ipc::message_t* msg = (ipc::message_t*)pool_.malloc();
        msg->length = ipc::MESSAGE_INIT_CAPACITY - sizeof(ipc::message_t);
        return std::shared_ptr<ipc::message_t>(msg, free_message);
    }
    else {
        length = (sizeof(ipc::message_t) + length + 4096 - 1) & ~(4096-1);
        ipc::message_t* msg = (ipc::message_t*)malloc(length);
        msg->length = length - sizeof(ipc::message_t);
        return std::shared_ptr<ipc::message_t>(msg, free);
    }
}

void ipc::relloc_message(std::shared_ptr<ipc::message_t>& msg, std::uint16_t copy, std::uint16_t length) {
    length = (sizeof(ipc::message_t) + length + 4096 - 1) & ~(4096-1);

    ipc::message_t* m = (ipc::message_t*)malloc(length);
    std::memcpy(m, msg.get(), sizeof(ipc::message_t) + copy);
    msg.reset(m, free);
}

std::uint32_t ipc::create_uid() {
    static std::uint32_t start = 0;
    return ++start;
}
