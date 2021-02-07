#ifndef CORE_CLOCK_H
#define CORE_CLOCK_H

#include "vendor.h"

namespace core {

    // 常用辅助函数：时间
    class clock: public boost::serialization::singleton<clock> {
    public:
        // 初始化时间计算参照
        clock();
        // 时间点
        operator std::chrono::system_clock::time_point() const;
        // 时间点
        std::chrono::system_clock::time_point time_point() const;
        // 毫秒
        operator std::int64_t() const;
        // 格式化（本地时间）
        std::string format(const char* style = "%Y-%m-%d %H:%M:%S") const;
        // 格式化（本地时间）
        void format(char* buffer, const char* style = "%Y-%m-%d %H:%M:%S") const;
        // Linux 时间戳
        std::time_t epoch() const;
        // 标准时间：YYYY-MM-DDTHH:II:SS.MSZ
        std::string utc() const;
        // 标准时间：YYYY-MM-DD HH:II:SS
        std::string iso() const;
    private:
        std::chrono::system_clock::time_point sys_; // 系统时间参照
        std::chrono::steady_clock::time_point sty_; // 稳定时间参照
    };
}

#endif // CORE_CLOCK_H
