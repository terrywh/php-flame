#include <chrono>
#ifndef NDEBUG
#include "vendor.h"
#endif

#include "coroutine.h"
#include "util.h"


#include <cstdint>
#include <boost/asio/steady_timer.hpp>
#include <fmt/format.h>
#include <fmt/chrono.h>

namespace core {
    static char buffer_[1024];
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
    // 全局时钟
    clock $clock;

    random_string::random_string(int size, std::string_view code)
    : random_genr(reinterpret_cast<std::uintptr_t>(this))
    , random_dist(code.size())
    , size_(size)
    , code_(code) {

    }
    // 生成一个随机字符串
    random_string::operator std::string() const {
        std::string str;
        str.resize(size_);
        build(str.data(), size_);
        return str;
    }
    // 在指定缓存空间中生成
    void random_string::build(char* const buffer, int size) const {
        if(size == 0) size = size_;
        for(int i=0;i<size;++i) {
            buffer[i] = code_[random_dist(random_genr)];
        }
        buffer[size] = '\0';
    }
    // 在公共缓存空间中生成
    const char* random_string::build(int size) const {
        if(size == 0) size = size_;
        assert(size < sizeof(buffer_)); // 公共缓冲空间大小
        build(buffer_, size);
        return buffer_;
    }
    // 协程：
    void co::sleep(boost::asio::io_context& io, std::chrono::milliseconds ms, coroutine_handler& ch) {
        boost::asio::steady_timer tm(io);
        tm.expires_after(ms);
        tm.async_wait(ch);
    }
}
