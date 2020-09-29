/********************************************************************
 * php-flame - PHP full stack development framework
 * copyright (c) 2020 terrywh
 ********************************************************************/

#ifndef PHP_FLAME_CORE_URL_H
#define PHP_FLAME_CORE_URL_H

#include <string>
#include <string_view>
#include <phpext/phpext.h>

struct Curl_URL;
typedef struct Curl_URL CURLU;

namespace core {
    // 解析或生成 URL 字符串
    class url {
    private:
        CURLU         *url_;
        mutable php::value   query_var_;  // 容器：query_
        php::array&  query_;  // 查询
    public:
        // 用于生成 URL 字符串
        url();
        // 用于解析 URL 字符串
        url(std::string_view str, bool parse_query=true);
        //
        ~url();
        // 协议
        std::string scheme() const;
        // 协议
        url& scheme(std::string_view protocol);
        // 重新拼接生成字符串
        std::string str(bool with_query = true) const;
        // 获取或设置查询
        php::array& query() const;
    };
    // 序列化
    std::ostream& operator << (std::ostream& os, const url& u);
}
#endif // PHP_FLAME_CORE_URL_H
