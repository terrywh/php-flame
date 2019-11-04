#include "master_ipc.h"
#include "master_logger.h"
#include "util.h"
#include "coroutine.h"

master_ipc::master_ipc(boost::asio::io_context& io)
: io_(io)
, server_(io, boost::asio::local::stream_protocol()) {
    svrsck_ = (boost::format("/tmp/flame_ipc_%d.sock") % ::getpid()).str();
}

master_ipc::~master_ipc() {
    ipc_close();
    std::filesystem::remove(svrsck_);
}

void master_ipc::ipc_start() {
    ::coroutine::start(io_.get_executor(), std::bind(&master_ipc::ipc_run, ipc_self(), std::placeholders::_1));
}
// !!! 工作线程中的协程
void master_ipc::ipc_run(coroutine_handler ch) {
    std::error_code ec;
    std::filesystem::remove(svrsck_, ec);
    boost::asio::local::stream_protocol::endpoint addr(svrsck_);
    server_.bind(addr);
    server_.listen();
    boost::system::error_code error;
    while(true) {
        auto sock = std::make_shared<boost::asio::local::stream_protocol::socket>(io_);
        server_.async_accept(*sock, ch[error]);
        if (error) break;
        coroutine::start(io_.get_executor(), std::bind(&master_ipc::ipc_read, ipc_self(), sock, std::placeholders::_1));
    }
    if (error && error != boost::asio::error::operation_aborted)
        output() << "[" << util::system_time() << "] (ERROR) Failed to accept M-IPC: (" << error.value() << ") " << error.message() << "\n";
    
    server_.close(error);
}

void master_ipc::ipc_close() {
    boost::system::error_code error;
    server_.close(error);
    for(auto i=socket_.begin();i!=socket_.end();++i) {
        i->second->close(error);
    }
}

void master_ipc::ipc_read(socket_ptr sock, coroutine_handler ch) {
    boost::system::error_code error;
    while(true) {
        auto msg = ipc::create_message();
        boost::asio::async_read(*sock, boost::asio::buffer(msg.get(), sizeof(ipc::message_t)), ch[error]);
        if (error) break;
        if (msg->length > ipc::MESSAGE_INIT_CAPACITY - sizeof(ipc::message_t)) {
            ipc::relloc_message(msg, 0, msg->length);
        }
        boost::asio::async_read(*sock, boost::asio::buffer(&msg->payload[0], msg->length), ch[error]);
        if (error) break;
        if (!on_message(msg, sock)) break;
    }
    if (error && error != boost::asio::error::operation_aborted && error != boost::asio::error::eof) 
        output() << "[" << util::system_time() << "] (ERROR) Failed to read M-IPC: (" << error.value() << ") " << error.message() << "\n";
    // 发生次数较少，效率不太重要了
    for(auto i=socket_.begin();i!=socket_.end();++i) {
        if(i->second == sock) {
            socket_.erase(i);
            break;
        }
    }
    sock->close(error);
}

bool master_ipc::on_message(std::shared_ptr<ipc::message_t> msg, socket_ptr sock) {
    if(msg->command >= ipc::COMMAND_TRANSFER_TO_CHILD) send(msg);
    else if(msg->command == ipc::COMMAND_REGISTER) socket_[msg->source] = sock;
    return true;
}

std::shared_ptr<ipc::message_t> master_ipc::ipc_request(std::shared_ptr<ipc::message_t> req, coroutine_handler& ch) {
    std::shared_ptr<ipc::message_t> res;
    callback_.emplace(req->unique_id, ipc::callback_t {ch, res});
    req->source = 0;
    send(req);
    ch.suspend();
    return res;
}

void master_ipc::ipc_request(std::shared_ptr<ipc::message_t> req) {
    req->source = 0;
    send(req);
}

void master_ipc::send(std::shared_ptr<ipc::message_t> msg) {
    boost::asio::post(io_, [this, self = ipc_self(), msg] () {
        if(sendq_.empty()) {
            sendq_.push_back(msg);
            send_next();
        }else
            sendq_.push_back(msg);
    });
}

void master_ipc::send_next() {
    if (sendq_.empty()) return;
    std::shared_ptr<ipc::message_t> msg = sendq_.front();
    auto i = socket_.find(msg->target);
    if(i == socket_.end()) {
        // std::cout << "message_dropped: " << (int) msg->command << " => " << (int) msg->target << "\n";
        sendq_.pop_front();
        boost::asio::post(io_, std::bind(&master_ipc::send_next, ipc_self()));
    }
    else {
        boost::asio::async_write(*i->second, boost::asio::buffer(msg.get(), sizeof(ipc::message_t) + msg->length), [this] (const boost::system::error_code& error, std::size_t size) {
            if(error) {
                output() << "[" << util::system_time() << "] [ERROR] Failed to write M-IPC: (" << error.value() << ") " << error.message() << "\n";
                return;
            }
            sendq_.pop_front();
            send_next();
        });
    }
}