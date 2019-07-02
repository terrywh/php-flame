#include "vendor.h"
#include "url.h"
#include <curl/curl.h>

typedef parser::separator_parser<std::string, std::string> parser_t;

url::url(const php::string &str, bool parse_query)
: raw_(str)
, port(0) { // 原始类型 port 初始值可能随机
    auto u = curl_url();
    curl_url_set(u, CURLUPART_URL, str.c_str(), CURLU_NON_SUPPORT_SCHEME);
    char * tmp;
    if(curl_url_get(u, CURLUPART_SCHEME, &tmp, 0) == CURLUE_OK) { // SCHEME
        schema.assign(tmp);
        curl_free(tmp);
    }
    if(curl_url_get(u, CURLUPART_USER, &tmp, 0) == CURLUE_OK) { // USER
        user.assign(tmp);
        curl_free(tmp);
    }
    if(curl_url_get(u, CURLUPART_PASSWORD, &tmp, 0) == CURLUE_OK) { // PASS
        pass.assign(tmp);
        curl_free(tmp);
    }
    if(curl_url_get(u, CURLUPART_HOST, &tmp, 0) == CURLUE_OK) { // USER
        host.assign(tmp);
        curl_free(tmp);
    }
    if(curl_url_get(u, CURLUPART_PORT, &tmp, 0) == CURLUE_OK) { // PORT
        port = std::atoi(tmp);
        curl_free(tmp);
    }
    if(curl_url_get(u, CURLUPART_PATH, &tmp, 0) == CURLUE_OK) { // PATH
        path.assign(tmp);
        curl_free(tmp);
    }
    if(parse_query && curl_url_get(u, CURLUPART_QUERY, &tmp, 0) == CURLUE_OK) { // QUERY
        parser_t p('\0', '\0', '=', '\0', '\0', '&', [this](parser_t::entry_type et) {
            php::url_decode_inplace(et.second.data(), et.second.size());
            query[et.first] = et.second;
            // std::cout << et.first << ": " << et.second << "\n";
        });
        p.parse(tmp, strlen(tmp));
        p.end();
        curl_free(tmp);
    }
}

std::string url::str(bool with_query, bool update) {
    if (update) {
        std::ostringstream ss;
        ss << schema << "://" << user << ":" << pass << "@" << host << ":" << port << path;
        if (with_query) {
            ss << "?";
            for (auto i=query.begin();i!=query.end();++i) {
                ss << i->first << "=" << php::url_encode(i->second.c_str(), i->second.size()) << "&";
            }
        }
        raw_ = ss.str();
    }
    return raw_;
}
