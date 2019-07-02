#pragma once

class logger {
public:
    logger(unsigned int index)
    : ref_(1)
    , idx_(index) { }

    virtual ~logger() = default;

    unsigned int addref() { return ++ref_; }
    unsigned int delref() { return --ref_; }

    unsigned int index() {
        return idx_;
    }
    virtual void reload() {}
    virtual std::ostream& stream() = 0;
    void write(std::string_view data, bool flush = true) {
        std::ostream& os = stream();
        os << data;
        if(flush) os.flush();
    }
private:
    unsigned int ref_;
    unsigned int idx_;
};

class logger_manager {
public:
    virtual unsigned int connect(const std::string& filepath) = 0;
    void reload() {
        for(auto i=logger_.begin();i!=logger_.end();++i) i->second->reload();
    }
    // 删除对应的 logger 引用
    void destroy(unsigned int w) {
        if(logger_[w]->delref() == 0) {
            logger_.erase(w);
        }
    }
    logger* index(unsigned int w) {
        return logger_[w].get();
    }
protected:
    std::map<unsigned int, std::unique_ptr<logger>> logger_;
};

