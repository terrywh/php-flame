#pragma once
#include "vendor.h"

class master_logger;
class master_logger_manager {
public:
    master_logger_manager();
    virtual ~master_logger_manager();
    // 日志引用
    master_logger* lm_connect(const std::string& filepath);
    // void lm_destroy(master_logger* ml) {
    //     assert(ml->ref_ > 0 && "引用计数异常");
    //     if(--ml->ref_ > 0) return;
    //     logger_.erase(ml->idx_);
    // }
    master_logger* lm_get(std::uint8_t idx);
    // 引用清理
    void lm_destroy(std::uint8_t idx);
    // 日志重载
    void lm_reload();
    void lm_close();
    virtual std::shared_ptr<master_logger_manager> lm_self() = 0;
private:
    std::uint8_t                                            index_ = 0;
    std::map<unsigned int, std::unique_ptr<master_logger>> logger_;
};