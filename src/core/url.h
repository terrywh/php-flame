#ifndef CORE_URL_H
#define CORE_URL_H

#include <curl/curl.h>
#include <string>
#include <string_view>
#include <cstring>

namespace core {
    // 解析或生成 URL 字符串
    class url {
    private:
        CURLU *url_;
    public:
        class part {
        public:
            part(part&& p)
            : data_(p.data_)
            , size_(p.size_) {
                p.data_ = nullptr;
                p.size_ = 0;
            }
            ~part() { curl_free(data_); } // no action if data_ == nullptr for curl_free
            operator std::string_view() const {
                return std::string_view(data_);
            }
            operator char**() {
                return &data_;
            }
            const char* c_str() const {
                return data_;
            }
            char* data() {
                return data_;
            }
            std::size_t size() const {
                return size_;
            }
        private:
            explicit part(char* data): data_(data), size_(std::strlen(data_)) {}
            explicit part(): data_(nullptr), size_(0ul) {}
            char*       data_;
            std::size_t size_;
            friend class url;
        };
        // 用于生成 URL 字符串
        url();
        // 用于解析 URL 字符串
        url(std::string_view str);
        // 
        ~url();

#define NAME(name) name
#define DECLARE_URL_PART_ACCESSOR(name)     \
    part NAME(name) () const;               \
    url& NAME(name) (std::string_view data)

        DECLARE_URL_PART_ACCESSOR(scheme);
        DECLARE_URL_PART_ACCESSOR(user);
        DECLARE_URL_PART_ACCESSOR(password);
        DECLARE_URL_PART_ACCESSOR(host);
        DECLARE_URL_PART_ACCESSOR(port);
        DECLARE_URL_PART_ACCESSOR(path);
        DECLARE_URL_PART_ACCESSOR(query);

#undef NAME
#undef DECLARE_URL_PART_ACCESSOR
        // 重新拼接生成字符串
        part str() const;
    };
    // 序列化
    std::ostream& operator << (std::ostream& os, const url& u);
}
#endif // CORE_URL_H
