#ifndef CORE_URL_H
#define CORE_URL_H

#include "vendor.h"

namespace core {

    class parsed_url {
    public:
        parsed_url(std::cmatch&& m)
        : m_(std::move(m)) {}

        inline std::string_view scheme() const {
            return gm(m_[1]);
        }
        inline std::string_view user() const {
            return gm(m_[3]);
        }
        inline std::string_view pass() const {
            return gm(m_[5]);
        }
        inline std::string_view host() const {
            return gm(m_[6]);
        }
        inline std::string_view port() const {
            return gm(m_[8]);
        }
        inline std::string_view path() const {
            return gm(m_[9]);
        }
        inline std::string_view query() const {
            return gm(m_[10]);
        }
    private:
        std::cmatch       m_;
        // 读取匹配子组对应字符串视图
        std::string_view gm(const std::csub_match& m) const {
            return {m.first, static_cast<std::size_t>(m.second - m.first)};
        }
    };

    // 解析或生成 URL 字符串
    parsed_url parse_url(std::string_view url);
}
#endif // CORE_URL_H
