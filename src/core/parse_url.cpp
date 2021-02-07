#include "parse_url.h"
#include <iostream>

namespace core {
    //
    static std::regex r { R"regex(^([^:]+)://(([^:@]+)(:([^@]+))?@)?([^:/]+)(:([^/]+))?(/[^\?]+)?(\?.+)?$)regex" };
    // 用于解析 URL 字符串
    parsed_url parse_url(std::string_view url) { // 原始类型 port 初始值可能随机
        std::cmatch m;
        if(!std::regex_match(url.begin(), url.end(), m, r)) {
            throw std::runtime_error("failed to parse url: ill-formed");
        }
        return std::move(m);
    }
}