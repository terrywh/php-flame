#include "clock.h"

namespace core {
    // 初始化时间计算参照
    clock::clock()
    : sys_(std::chrono::system_clock::now())
    , sty_(std::chrono::steady_clock::now()) {}
    // 时间点
    clock::operator std::chrono::system_clock::time_point() const {
        return (std::chrono::steady_clock::now() - sty_) + sys_;
    }
    // 时间点
    std::chrono::system_clock::time_point clock::time_point() const {
        return (std::chrono::steady_clock::now() - sty_) + sys_;
    }
    // 毫秒（时间戳）
    clock::operator std::int64_t() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                static_cast<std::chrono::system_clock::time_point>(*this).time_since_epoch()
            ).count();
    }
    // 格式化时间
    std::string clock::format(const char* style) const {
        std::string s(19, '\0');
        format(s.data(), style);
        return s;
    }
    // 格式化时间
    void clock::format(char* buffer, const char* style) const {
        auto tp = static_cast<std::chrono::system_clock::time_point>(*this);
        auto tt = std::chrono::system_clock::to_time_t(tp);
        fmt::format_to(buffer, style, *std::localtime(&tt));
    }
    // Linux 时间戳
    std::time_t clock::epoch() const {
        return std::chrono::system_clock::to_time_t(
            static_cast<std::chrono::system_clock::time_point>(*this));
    }

    // YYYY-MM-DDTHH:II:SS.SSSZ
    std::string clock::utc() const {
        auto now = time_point();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::tm*    tm = std::gmtime(&tt);
        return fmt::format("{:%FT%H:%M}:{:%S}Z", *tm, now.time_since_epoch());
    }
    // YYYY-MM-DD HH:II:SS
    std::string clock::iso() const {
        auto now = time_point();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::tm*    tm = std::gmtime(&tt);
        return fmt::format("{:%F %T}", *tm);
    }
}
