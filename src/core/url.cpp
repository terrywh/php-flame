#include "vendor.h"
#include "url.h"
#include <curl/curl.h>

namespace core {
    // 用于生成 URL 字符串
    url::url()
    : url_(curl_url())
    , query_var_(php::array::create(4))
    , query_(query_var_) {

    }
    // 用于解析 URL 字符串
    url::url(std::string_view str, bool parse_query)
    : url_(curl_url())
    , query_var_(php::array::create(4))
    , query_(query_var_) { // 原始类型 port 初始值可能随机
        curl_url_set(url_, CURLUPART_URL, str.data(), CURLU_NON_SUPPORT_SCHEME);
        char * tmp;
        // if(curl_url_get(u, CURLUPART_SCHEME, &tmp, 0) == CURLUE_OK) { // SCHEME
        //     schema_.assign(tmp);
        //     curl_free(tmp);
        // }
        // if(curl_url_get(u, CURLUPART_USER, &tmp, 0) == CURLUE_OK) { // USER
        //     user.assign(php::url_decode(tmp));
        //     curl_free(tmp);
        // }
        // if(curl_url_get(u, CURLUPART_PASSWORD, &tmp, 0) == CURLUE_OK) { // PASS
        //     pass.assign(php::url_decode(tmp));
        //     curl_free(tmp);
        // }
        // if(curl_url_get(u, CURLUPART_HOST, &tmp, 0) == CURLUE_OK) { // HOST
        //     host.assign(tmp);
        //     curl_free(tmp);
        // }
        // if(curl_url_get(u, CURLUPART_PORT, &tmp, 0) == CURLUE_OK) { // PORT
        //     port = std::atoi(tmp);
        //     curl_free(tmp);
        // }
        // if(curl_url_get(u, CURLUPART_PATH, &tmp, 0) == CURLUE_OK) { // PATH
        //     path.assign(tmp);
        //     curl_free(tmp);
        // }
        if(parse_query && curl_url_get(url_, CURLUPART_QUERY, &tmp, 0) == CURLUE_OK) { // QUERY
            php::parse_form_data(tmp, query_var_);
            curl_free(tmp);
        }
    }
    url::~url() {
        curl_url_cleanup(url_);
    }
    // 协议
    std::string url::scheme() const {
        std::string schema;
        char * tmp;
        if(curl_url_get(url_, CURLUPART_SCHEME, &tmp, 0) == CURLUE_OK) { // SCHEME
            schema.assign(tmp);
            curl_free(tmp);
        }
        return schema;
    }
    // 协议
    url& url::scheme(std::string_view proto) {
        curl_url_set(url_, CURLUPART_SCHEME, proto.data(), 0);
        return *this;
    }
    // 获取或设置查询
    php::array& url::query() {
        return query_;
    }
    // 重新拼接生成字符串
    std::string url::str(bool with_query) {
        std::string u;
        char* tmp, *q = nullptr;
        if(!with_query && curl_url_get(url_, CURLUPART_QUERY, &q, 0) == CURLUE_OK) {
            // 暂时屏蔽 QUERY 部分
            curl_url_set(url_, CURLUPART_QUERY, nullptr, 0);
        }
        if(curl_url_get(url_, CURLUPART_URL, &tmp, 0) == CURLUE_OK) {
            u.assign(tmp);
            curl_free(tmp);
        }
        if(q) {
            curl_url_set(url_, CURLUPART_QUERY, q, 0);
            curl_free(q);
        }
        return u;
    }
}