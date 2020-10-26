#ifndef PHP_FLAME_CORE_UTIL_H
#define PHP_FLAME_CORE_UTIL_H

#include <chrono>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <string_view>

#include <cstdint>

namespace flame { namespace core {

    template <class T>
    class basic_coroutine_handler;
    class coroutine;
    using coroutine_handler = basic_coroutine_handler<coroutine>;
    // 常用辅助函数：时间
    class clock {
    public:
        // 初始化时间计算参照
        clock();
        // 时间点
        operator std::chrono::system_clock::time_point() const {
            return (std::chrono::steady_clock::now() - sty_) + sys_;
        }
        // 秒
        operator std::int32_t() const {
            return std::chrono::duration_cast<std::chrono::seconds>(
                static_cast<std::chrono::system_clock::time_point>(*this).time_since_epoch() ).count();
        }
        // 毫秒
        operator std::int64_t() const {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                static_cast<std::chrono::system_clock::time_point>(*this).time_since_epoch() ).count();
        }
        // YYYY-mm-dd HH:ii:SS
        operator std::string() const;
        // 在指定缓存空间构建 YYYY-mm-dd HH:ii:SS
        void build(char* const buffer);
        // 在公共缓存空间构建 YYYY-mm-dd HH:ii:SS
        const char* build();
    private:
        std::chrono::system_clock::time_point sys_; // 系统时间参照
        std::chrono::steady_clock::time_point sty_; // 稳定时间参照
    };
    // 全局时钟
    extern clock $clock;
    // 随机字符串
    class random {
    public:
        random();
        // 使用指定码表构建指定长度的随机字符串
        std::string string(int size = 16, std::string_view code = "ABCDEFGHIJKLMKOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") const;
        // 在指定缓存空间中生成（可临时调整生成长度）
        void to(char* const buffer, int size = 16, std::string_view code = "ABCDEFGHIJKLMKOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") const;
        // 在公共缓存空间中生成（可临时调整生成长度）
        const char* make(int size = 16, std::string_view code = "ABCDEFGHIJKLMKOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") const;
        // 数值
        int i32(int min = 0, int max = std::numeric_limits<int>::max());
    private:
        mutable std::default_random_engine      random_genr;
        mutable std::uniform_int_distribution<> random_dist;
    };
    // 全局时钟
    extern random $random;
}}

#endif // PHP_FLAME_CORE_UTIL_H
