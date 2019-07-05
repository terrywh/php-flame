#include "master_logger_manager.h"

master_logger_manager::master_logger_manager(boost::asio::io_context& io)
: io_(io) {

}

master_logger_manager::~master_logger_manager() {
    // std::cout << "~master_logger_manager\n";
}

master_logger* master_logger_manager::lm_connect(std::string_view filepath) {
    std::filesystem::path path = filepath;
    
    for(auto i=logger_.begin();i!=logger_.end();++i) {
        if(static_cast<master_logger*>(i->second.get())->path_ == path) {
            ++i->second->ref_;
            return i->second.get();
        }
    }
    auto p = logger_.insert({index_, std::make_unique<master_logger>(path, index_)});
    ++index_;
    p.first->second->reload(io_);
    return p.first->second.get();
}

void master_logger_manager::lm_destroy(std::uint8_t idx) {
    auto i = logger_.find(idx);
    if(i == logger_.end()) return;
    assert(i->second->ref_ > 0 && "引用计数异常");
    if(--i->second->ref_ > 0) return;
    logger_.erase(i);
}

void master_logger_manager::lm_reload() {
    for(auto i=logger_.begin();i!=logger_.end();++i) i->second->reload(io_);
}

void master_logger_manager::lm_close() {
    for(auto i=logger_.begin();i!=logger_.end();++i) i->second->close();
}