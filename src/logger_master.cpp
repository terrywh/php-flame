#include "logger_master.h"

void logger_master::reload() {
    file_.reset(new std::ofstream(path_, std::ios_base::out | std::ios_base::app));
    if (file_->fail()) // 文件打开失败时不会抛出异常，需要额外的状态检查
        std::cerr << "(ERROR) failed to create/open logger: file cannot be created or accessed\n";
    else return;
    file_.reset(&std::clog, boost::null_deleter());
}

unsigned int logger_manager_master::connect(const std::string& filepath) {
    std::filesystem::path path = filepath;
    
    for(auto i=logger_.begin();i!=logger_.end();++i) {
        if(static_cast<logger_master*>(i->second.get())->path_ == path) {
            i->second->addref();
            return i->second->index();
        }
    }
    auto p = logger_.insert({index_, std::make_unique<logger_master>(path, index_)});
    ++index_;
    p.first->second->reload();
    return p.first->second->index();
}
