#pragma once
#include "vendor.h"

class logger {
public:
    logger(unsigned int index)
    : ref_(1)
    , idx_(index) {
        

    }

    virtual ~logger() = default;

    unsigned int addref() { return ++ref_; }
    unsigned int delref() { return --ref_; }

    unsigned int index() {
        return idx_;
    }
    virtual void reload() {}
    virtual std::ostream& stream() = 0;
    virtual void write(std::string_view data, bool flush = true) = 0;
private:
    unsigned int ref_;
    unsigned int idx_;

};

class coroutine_handler;
class logger_manager {
public:
    virtual ~logger_manager() = default;
    virtual logger* connect(const std::string& filepath, coroutine_handler& ch) = 0;
    void reload() {
        for(auto i=logger_.begin();i!=logger_.end();++i) i->second->reload();
    }
    // 删除对应的 logger 引用
    void destroy(logger* l) {
        if(l->delref() == 0) logger_.erase(l->index());
    }
    logger* index(unsigned int w) {
        return logger_.empty() ? nullptr : logger_[w].get();
    }
protected:
    std::map<unsigned int, std::unique_ptr<logger>> logger_;
};

