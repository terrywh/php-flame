#include "random_string.h"
#include "context.h"

namespace core {

    random_string::random_string(const std::string& table)
    : stable_( table )
    , distri_( 0, stable_.size() ) {

    }
    // 使用指定码表构建指定长度的随机字符串
    std::string random_string::make(std::size_t size) const {
        std::string s(size, '\0');
        make(s.data(), size);
        return s;
    }
    // 在指定缓存空间中生成（可临时调整生成长度）
    void random_string::make(char* const buffer, std::size_t size) const {
        for(int i=0;i<size;++i) {
            int r = distri_($context->random) % size;
            buffer[i] = stable_[r];
        }
        buffer[size] = '\0';
    }
    // 在公共缓存空间中生成（可临时调整生成长度）
    std::string_view random_string::build(std::size_t size) const {
        static char buffer[256];
        make(buffer, size);
        return std::string_view(buffer, size);
    }
}