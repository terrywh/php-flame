#ifndef CORE_RANDOM_H
#define CORE_RANDOM_H

#include <limits>
#include <random>
#include <string>
#include <string_view>
#include <cstdint>

namespace core {

    // 随机字符串
    class random_string {
    public:
        random_string(const std::string& table = 
            "ABCDEFGHIJKLMKOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
        // 使用指定码表构建指定长度的随机字符串
        std::string make(std::size_t size = 16) const;
        // 在指定缓存空间中生成（可临时调整生成长度）
        void make(char* const buffer, std::size_t size = 16) const;
        // 在公共缓存空间中生成（可临时调整生成长度）
        std::string_view build(std::size_t size = 16) const;
    private:
        const std::string&                         stable_;
        mutable std::uniform_int_distribution<int> distri_;
    };
}

#endif // CORE_RANDOM_H
