#include "ipc.h"

static boost::pool<> pool_(ipc::MESSAGE_INIT_CAPACITY);

ipc::message_t* ipc::malloc_message(std::uint16_t length) {
    ipc::message_t* msg;
    if(length > ipc::MESSAGE_INIT_CAPACITY - sizeof(ipc::message_t)) {
        length = (length + sizeof(ipc::message_t) + 4096 - 1) & ~(4096-1);
        msg = (ipc::message_t*)malloc(length);
        msg->length = length - sizeof(ipc::message_t);
    }
    else {
        msg = (ipc::message_t*)malloc(ipc::MESSAGE_INIT_CAPACITY);
        msg->length = ipc::MESSAGE_INIT_CAPACITY - sizeof(ipc::message_t);
    }
    return msg;
}

void ipc::free_message(ipc::message_t* msg) {
    if(msg->length > ipc::MESSAGE_INIT_CAPACITY - sizeof(ipc::message_t)) 
        free(msg);
    else
        pool_.free(msg);
}

std::shared_ptr<ipc::message_t> ipc::create_message() {
    return std::shared_ptr<ipc::message_t>(ipc::malloc_message(), ipc::free_message);
}

void ipc::relloc_message(std::shared_ptr<ipc::message_t>& msg, std::uint16_t copy, std::uint16_t length) {
    ipc::message_t* m = malloc_message(length);
    std::memcpy(m, msg.get(), sizeof(ipc::message_t) + copy);
    msg.reset(m, free_message);
}

std::uint32_t ipc::create_uid() {
    static std::uint32_t start = 0;
    return ++start;
}
