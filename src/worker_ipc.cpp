#include "worker_ipc.h"
#include "util.h"

worker_ipc::worker_ipc(boost::asio::io_context& io)
: socket_(new boost::asio::local::stream_protocol::socket(io)) {

}

worker_ipc::~worker_ipc() {
    std::cout << "~worker_ipc\n";
    ipc_close();
}

std::shared_ptr<ipc::message_t> worker_ipc::ipc_request(std::shared_ptr<ipc::message_t> req, coroutine_handler& ch) {
    std::shared_ptr<ipc::message_t> res;
    callback_.emplace(req->unique_id, ipc::callback_t {ch, res});
    send(req);
    std::cout << "before suspend\n";
    ch.suspend();
    std::cout << "after suspend\n";
    return res;
}

void worker_ipc::ipc_request(std::shared_ptr<ipc::message_t> data) {
    send(data);
}

static void json_message_free(ipc::message_t* msg) {
    zend_string* ptr = reinterpret_cast<zend_string*>((char*)(msg) - offsetof(zend_string, val));
    smart_str str { ptr, 0 };
    smart_str_free(&str);
}

void worker_ipc::ipc_notify(std::uint8_t target, php::value data) {
    smart_str str; // !!!! 避免 JSON 文本的 COPY 复制
    smart_str_alloc(&str, 1, false);
    ipc::message_t* msg = (ipc::message_t*)str.s->val;
    msg->command = ipc::COMMAND_NOTIFY_DATA;
    msg->target  = target;
    str.s->len = sizeof(ipc::message_t);
    php::json_encode_to(&str, data);
    msg->length = str.s->len - sizeof(ipc::message_t);
    send(std::shared_ptr<ipc::message_t>{msg, ipc::free_message});
}

void worker_ipc::ipc_run(coroutine_handler ch) {
    // TODO
    // 连接主进程 IPC 通道
    // 发送注册消息
    // 
    boost::system::error_code error;
    while(true) {
        auto msg = ipc::create_message();
        boost::asio::async_read(*socket_, boost::asio::buffer(msg.get(), sizeof(ipc::message_t)), ch[error]);
        if(error) break;
        if(msg->length > ipc::MESSAGE_INIT_CAPACITY - sizeof(ipc::message_t)) {
            ipc::relloc_message(msg, 0, msg->length);
        }
        boost::asio::async_read(*socket_, boost::asio::buffer(&msg->payload[0], msg->length), ch[error]);
        if(error) break;

        switch(msg->command) {
        case ipc::COMMAND_LOGGER_CONNECT: {
            auto pcb = callback_.extract(msg->unique_id);
            assert(!pcb.empty() && "未找到必要回调");
            pcb.mapped().res = msg;
            pcb.mapped().cch.resume();
        }
        break;
        case ipc::COMMAND_NOTIFY_DATA:
            on_notify(msg);
        break;
        }
    }

    if (error && error != boost::asio::error::operation_aborted && error != boost::asio::error::eof) 
        output() << "[" << util::system_time() << "] (ERROR) Failed to read ipc connection: (" << error.value() << ") " << error.message() << "\n";
    socket_->close(error);
}

void worker_ipc::ipc_close() {
    socket_->close();
}

void worker_ipc::send(std::shared_ptr<ipc::message_t> msg) {
    
    if(sendq_.empty()) {
        sendq_.push_back(msg);
        send_next();
    }else
        sendq_.push_back(msg);
}

void worker_ipc::send_next() {
    std::shared_ptr<ipc::message_t> msg = sendq_.front();

    boost::asio::async_write(*socket_, boost::asio::buffer(msg.get(), sizeof(ipc::message_t) + msg->length), [this] (const boost::system::error_code& error, std::size_t size) {
        if(error) {
            output() << "[" << util::system_time() << "] [FATAL] Failed to write ipc message: (" << error.value() << ") " << error.message() << "\n";
            return;
        }
        sendq_.pop_front();
        send_next();
    });
}
