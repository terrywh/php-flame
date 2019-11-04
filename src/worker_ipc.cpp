#include "worker_ipc.h"
#include "util.h"

worker_ipc::worker_ipc(boost::asio::io_context& io, std::uint8_t idx)
: io_(io)
, socket_(new boost::asio::local::stream_protocol::socket(io))
, idx_(idx) {
    svrsck_ = (boost::format("/tmp/flame_ipc_%d.sock") % ::getppid()).str();
}

worker_ipc::~worker_ipc() {
    // std::cout << "~worker_ipc\n";
    ipc_close();
}

std::shared_ptr<ipc::message_t> worker_ipc::ipc_request(std::shared_ptr<ipc::message_t> req, coroutine_handler& ch) {
    std::shared_ptr<ipc::message_t> res;
    callback_.emplace(req->unique_id, ipc::callback_t {ch, res});
    req->source = idx_;
    send(req);
    ch.suspend();
    return res;
}

void worker_ipc::ipc_request(std::shared_ptr<ipc::message_t> req) {
    req->source = idx_;
    send(req);
}

static void free_json_message(ipc::message_t* msg) {
    zend_string* ptr = reinterpret_cast<zend_string*>((char*)(msg) - offsetof(zend_string, val));
    smart_str str { ptr, 0 };
    smart_str_free(&str);
}

void worker_ipc::ipc_notify(std::uint8_t target, php::value data) {
    if(data.type_of(php::TYPE::STRING)) {
        php::string str = data;
        auto msg = ipc::create_message(str.size());
        msg->command = ipc::COMMAND_MESSAGE_STRING;
        msg->source  = idx_;
        msg->target  = target;
        std::memcpy(&msg->payload[0], str.data(), str.size());
        msg->length = str.size();
        send(msg);
    }
    else {
        smart_str str {nullptr, 0}; // !!!! 避免 JSON 文本的 COPY 复制
        smart_str_alloc(&str, 1, false);
        ipc::message_t* msg = (ipc::message_t*)str.s->val;
        msg->command = ipc::COMMAND_MESSAGE_JSON;
        msg->source  = idx_;
        msg->target  = target;
        str.s->len = sizeof(ipc::message_t);
        php::json_encode_to(&str, data);
        msg->length = str.s->len - sizeof(ipc::message_t);
        send(std::shared_ptr<ipc::message_t>(msg, free_json_message));
    }
}

void worker_ipc::ipc_start() {
    // !!!! 注意本地日志的判定机制与时序、时间有关
    ++status_; // 0
    coroutine::start(io_.get_executor(), 
            std::bind(&worker_ipc::ipc_run, ipc_self(), std::placeholders::_1)); // 由于线程池影响，特殊的启动方式
}
// !!!! 工作线程
void worker_ipc::ipc_run(coroutine_handler ch) {
    boost::system::error_code error;
    std::shared_ptr<ipc::message_t> msg;
    std::uint64_t tmp;
    
    // 连接主进程 IPC 通道
    boost::asio::local::stream_protocol::endpoint addr(svrsck_);
    socket_->async_connect(addr, ch[error]);
    if (error) goto IPC_FAILED;
    ++status_; // 1
    // 发送注册消息
    msg = ipc::create_message();
    msg->command = ipc::COMMAND_REGISTER;
    msg->source  = idx_;
    msg->target  = 0;
    msg->length  = 0;
    boost::asio::async_write(*socket_, boost::asio::buffer(msg.get(), sizeof(ipc::message_t) + msg->length), ch[error]);
    if (error) goto IPC_FAILED;
    ++status_; // 2
    // 已经缓冲在队列中的数据开始发送
    send_next();
    while(true) {
        msg = ipc::create_message();
        boost::asio::async_read(*socket_, boost::asio::buffer(msg.get(), sizeof(ipc::message_t)), ch[error]);
        if(error) break;
        if(msg->length > ipc::MESSAGE_INIT_CAPACITY - sizeof(ipc::message_t)) {
            ipc::relloc_message(msg, 0, msg->length + 1); // 额外用于容纳结尾 '\0'
        }
        if(msg->length > 0) {
            boost::asio::async_read(*socket_, boost::asio::buffer(&msg->payload[0], msg->length), ch[error]);
            if(error) break;
            msg->payload[msg->length] = '\0'; // 防止 JSON 解析异常
        }
        if(!on_message(msg)) break;
        auto pcb = callback_.extract(msg->unique_id);
        if(!pcb.empty()) {
            pcb.mapped().res = msg;
            pcb.mapped().cch.resume();
        }
    }
IPC_FAILED:
    if (error && error != boost::asio::error::operation_aborted && error != boost::asio::error::eof) 
        output() << "[" << util::system_time() << "] (ERROR) Failed to read W-IPC: (" << error.value() << ") " << error.message() << "\n";
    socket_->close(error);
}

void worker_ipc::ipc_close() {
    socket_->close();
}

bool worker_ipc::ipc_enabled() {
    return status_ > -1;
}
// 由于实际调用者来自主线程，需要线性转接到（多个）工作线程中
void worker_ipc::send(std::shared_ptr<ipc::message_t> msg) {
    assert(status_ > -1);
    boost::asio::post(io_, [this, self = ipc_self(), msg] () {
        sendq_.push_back(msg);
        if(status_ > 1 && sendq_.size() == 1) send_next();
        // C++11 后， std::list::size() 是常数及时间复杂度
    });
}

void worker_ipc::send_next() {
    if (sendq_.empty()) return;
    std::shared_ptr<ipc::message_t> msg = sendq_.front();

    boost::asio::async_write(*socket_, boost::asio::buffer(msg.get(), sizeof(ipc::message_t) + msg->length), [this] (const boost::system::error_code& error, std::size_t size) {
        if(error) {
            output() << "[" << util::system_time() << "] [ERROR] Failed to write W-IPC: (" << error.value() << ") " << error.message() << "\n";
            return;
        }
        sendq_.pop_front();
        send_next();
    });
}
