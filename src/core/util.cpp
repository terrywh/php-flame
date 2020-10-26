#include "coroutine.hpp"
#include "util.hpp"

#include <fmt/format.h>
#include <fmt/chrono.h>

namespace flame { namespace core {
    static char buffer_[1024];

    clock $clock;
    // 初始化时间计算参照
    clock::clock()
    : sys_(std::chrono::system_clock::now())
    , sty_(std::chrono::steady_clock::now()) {}
    // YYYY-mm-dd HH:ii:SS
    clock::operator std::string() const {
        auto tp = static_cast<std::chrono::system_clock::time_point>(*this);
        auto tt = std::chrono::system_clock::to_time_t(tp);

        return fmt::format("%Y-%m-%d %H:%M:%S", fmt::localtime(tt));
    }
    // 在指定缓存空间构建 YYYY-mm-dd HH:ii:SS
    void clock::build(char* const buffer) {
        auto tp = static_cast<std::chrono::system_clock::time_point>(*this);
        auto tt = std::chrono::system_clock::to_time_t(tp);
        fmt::format_to(buffer, "%Y-%m-%d %H:%M:%S", fmt::localtime(tt));
    }
    // 在公共缓存空间构建 YYYY-mm-dd HH:ii:SS
    const char* clock::build() {
        build(buffer_);
        return buffer_;
    }
    
    random $random;

    random::random()
    : random_genr(reinterpret_cast<std::uintptr_t>(this)) {}
    
    std::string random::string(int size, std::string_view code) const {
        std::string str;
        str.resize(size);
        to(str.data(), size);
        return str;
    }
    // 在指定缓存空间中生成
    void random::to(char* const buffer, int size, std::string_view code) const {
        for(int i=0;i<size;++i) {
            int r = random_dist(random_genr) % size;
            buffer[i] = code[r];
        }
        buffer[size] = '\0';
    }

    const char* random::make(int size, std::string_view code) const {
        to(buffer_, size, code);
        return buffer_;
    }
    // 数值
    int random::i32(int min , int max) {
        return (random_dist(random_genr) % (max - min)) + min;
    }
}}