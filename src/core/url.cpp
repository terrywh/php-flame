#include "url.h"
#include "exception.h"
#include <iostream>

namespace core {
    // 用于生成 URL 字符串
    url::url()
    : url_(curl_url()) {

    }
    // 用于解析 URL 字符串
    url::url(std::string_view str)
    : url_(curl_url()) { // 原始类型 port 初始值可能随机
        curl_url_set(url_, CURLUPART_URL, str.data(), CURLU_NON_SUPPORT_SCHEME);
    }
    // 
    url::~url() {
        curl_url_cleanup(url_);
    }
#define DEFINE_URL_PART_ACCESSOR(name, which) \
    url::part url:: name() const {                                                    \
        part data;                                                                     \
        if(CURLUcode e = curl_url_get(url_, which, data, 0); e!= CURLUE_OK)            \
            raise<unknown_error>("failed to get scheme of url: code = {}", e);         \
        return data;                                                                   \
    }                                                                                  \
    url& url:: name(std::string_view data) {                                          \
        if(CURLUcode e = curl_url_set(url_, which, data.data(), 0); e != CURLUE_OK)    \
            raise<unknown_error>("failed to set " #name " of url: code = {}", e);      \
        return *this;                                                                  \
    }

    DEFINE_URL_PART_ACCESSOR(scheme,   CURLUPART_SCHEME)
    DEFINE_URL_PART_ACCESSOR(user,     CURLUPART_USER)
    DEFINE_URL_PART_ACCESSOR(password, CURLUPART_PASSWORD)
    DEFINE_URL_PART_ACCESSOR(host,     CURLUPART_HOST)
    DEFINE_URL_PART_ACCESSOR(path,     CURLUPART_PATH)
    DEFINE_URL_PART_ACCESSOR(query,    CURLUPART_QUERY)

#undef DEFINE_URL_PART_ACCESSOR

    // 重新拼接生成字符串
    url::part url::str() const {
        part tmp;
        if(CURLUcode code = curl_url_get(url_, CURLUPART_URL, tmp, 0); code != CURLUE_OK) 
            raise<unknown_error>("failed to generate url: code = {}", code);
        return tmp;
    }
    // 序列化
    std::ostream& operator << (std::ostream& os, const url& u) {
        url::part s = u.str();
        os.write(s.data(), s.size());
        return os;
    }
}