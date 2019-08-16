#include "../coroutine.h"
#include "value_body.h"
#include "client_request.h"
#include "client_body.h"
#include "http.h"

namespace flame::http {

    void client_request::declare(php::extension_entry& ext) {
        php::class_entry<client_request> class_client_request(
            "flame\\http\\client_request");
        class_client_request
            .constant({"HTTP_VERSION_1_0", CURL_HTTP_VERSION_1_0})
            .constant({"HTTP_VERSION_1_1", CURL_HTTP_VERSION_1_1})
            .constant({"HTTP_VERSION_2", CURL_HTTP_VERSION_2})
            .constant({"HTTP_VERSION_2_0", CURL_HTTP_VERSION_2_0})
            .constant({"HTTP_VERSION_2_TLS", CURL_HTTP_VERSION_2TLS})
            .constant({"HTTP_VERSION_2_PRI", CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE})
            .constant({"SSL_VERIFY_NONE",   0})
            .constant({"SSL_VERIFY_PEER",   1}) // CURLOPT_SSL_VERIFYPEER
            .constant({"SSL_VERIFY_HOST",   2}) // CURLOPT_SSL_VERIFYHOST
            .constant({"SSL_VERIFY_STATUS", 4}) // CURLOPT_SSL_VERIFYSTATUS
            .constant({"SSL_VERIFY_ALL",    7})
            .property({"timeout", 3000})
            .property({"method", "GET"})
            .property({"url", nullptr})
            .property({"header", nullptr})
            .property({"cookie", nullptr})
            .property({"body", ""})
            .method<&client_request::__construct>("__construct", {
                {"url", php::TYPE::STRING},
                {"body", php::TYPE::UNDEFINED, false, true},
                {"timeout", php::TYPE::INTEGER, false, true},
            })
            .method<&client_request::http_version>("http_version", {
                {"version", php::TYPE::INTEGER},
            })
            .method<&client_request::ssl_pem>("ssl_pem", {
                {"cert", php::TYPE::STRING},
                {"pkey", php::TYPE::STRING, false, true},
            })
            .method<&client_request::ssl_verify>("ssl_verify", {
                {"verify", php::TYPE::INTEGER},
            });

        ext.add(std::move(class_client_request));
    }

    php::value client_request::__construct(php::parameters& params) {
        if (params.length() > 2) set("timeout", params[2]);
        else set("timeout", 3000);

        if (params.length() > 1 && !params[1].empty()) {
            set("body", params[1]);
            set("method", "POST");
        }
        else set("method", "GET");
        set("url", params[0]);
        set("header", php::array(0));
        set("cookie", php::array(0));

        c_easy_ = curl_easy_init();
        return nullptr;
    }
    client_request::client_request()
    : c_head_(nullptr)
    /*, c_easy_(nullptr) */ {}
    client_request::~client_request() {
        if (c_head_) curl_slist_free_all(c_head_);
        if (c_easy_) curl_easy_cleanup(c_easy_); // 未执行的请求需要清理
    }

    php::value client_request::http_version(php::parameters& params) {
        long v = static_cast<int>(params[0]);
        if (v <= CURL_HTTP_VERSION_NONE || v >= CURL_HTTP_VERSION_LAST)
            throw php::exception(zend_ce_error_exception
                , "Failed to set HTTP version: value out of range"
                , -1);

        curl_easy_setopt(c_easy_, CURLOPT_HTTP_VERSION, v);
        return nullptr;
    }

    php::value client_request::ssl_pem(php::parameters& params) {
        curl_easy_setopt(c_easy_, CURLOPT_SSLCERTTYPE, "PEM");
        php::string cert = params[0];
        curl_easy_setopt(c_easy_, CURLOPT_SSLCERT, cert.c_str());
        if (params.size() > 1 && params[1].type_of(php::TYPE::STRING)) {
            php::string pkey = params[1];
            curl_easy_setopt(c_easy_, CURLOPT_SSLKEY, pkey.c_str());
        }
        if (params.size() > 2 && params[2].type_of(php::TYPE::STRING)) {
            php::string pass = params[2];
            curl_easy_setopt(c_easy_, CURLOPT_KEYPASSWD, pass.c_str());
        }
        return nullptr;
    }

    php::value client_request::ssl_verify(php::parameters& params) {
        long v = static_cast<int>(params[0]), YES = 1, NO = 0;
        if (v | CURLOPT_SSL_VERIFYPEER)
            curl_easy_setopt(c_easy_, CURLOPT_SSL_VERIFYPEER, YES);
        else
            curl_easy_setopt(c_easy_, CURLOPT_SSL_VERIFYPEER, NO);
        if (v | CURLOPT_SSL_VERIFYHOST)
            curl_easy_setopt(c_easy_, CURLOPT_SSL_VERIFYHOST, YES);
        else
            curl_easy_setopt(c_easy_, CURLOPT_SSL_VERIFYHOST, NO);
        if (v | CURLOPT_SSL_VERIFYSTATUS)
            curl_easy_setopt(c_easy_, CURLOPT_SSL_VERIFYSTATUS, YES);
        else
            curl_easy_setopt(c_easy_, CURLOPT_SSL_VERIFYSTATUS, NO);
        return nullptr;
    }

    void client_request::build_ex() {
        long timeout = static_cast<long>(get("timeout"));
        curl_easy_setopt(c_easy_, CURLOPT_TIMEOUT_MS, timeout);
        // 目标请求地址
        // ---------------------------------------------------------------------------
        php::string u = get("url");
        if (!u.type_of(php::TYPE::STRING))
            throw php::exception(zend_ce_type_error
                , "Failed to build client request: 'url' typeof 'string' required"
                , -1);

        curl_easy_setopt(c_easy_, CURLOPT_URL, u.c_str());
        php::string m = get("method");
        curl_easy_setopt(c_easy_, CURLOPT_CUSTOMREQUEST, m.c_str());
        // 头
        // ---------------------------------------------------------------------------
        if (c_head_ != nullptr) {
            curl_slist_free_all(c_head_);
            c_head_ = nullptr;
        }
        std::string ctype;
        long keepalive = 1;
        php::array header = get("header");
        for (auto i=header.begin(); i!=header.end(); ++i) {
            php::string key = i->first.to_string();
            php::string val = i->second.to_string();
            if (strncasecmp(key.c_str(), "content-type", 12) == 0) ctype.assign(val.data(), val.size());
            else if (strncasecmp(key.c_str(), "connection", 10) == 0
                && strncasecmp(val.c_str(), "close", 5) == 0) keepalive = 0;
            c_head_ = curl_slist_append(c_head_, (boost::format("%s: %s") % key % val).str().c_str());
        }
        c_head_ = curl_slist_append(c_head_, "Expect: ");
        curl_easy_setopt(c_easy_, CURLOPT_HTTPHEADER, c_head_);
        curl_easy_setopt(c_easy_, CURLOPT_TCP_KEEPALIVE, keepalive);
        // COOKIE
        // ---------------------------------------------------------------------------
        php::array cookie = get("cookie");
        php::buffer cookies;
        for (auto i=cookie.begin(); i!=cookie.end(); ++i) {
            php::string key = i->first.to_string();
            php::string val = i->second.to_string();
            val = php::url_encode(val.c_str(), val.size());
            cookies.append(key);
            cookies.push_back('=');
            cookies.append(val);
            cookies.push_back(';');
            cookies.push_back(' ');
        }
        php::string cookie_str = std::move(cookies);
        if (cookie_str.size() > 0) curl_easy_setopt(c_easy_, CURLOPT_COOKIE, cookie_str.c_str());
        // 体
        // ---------------------------------------------------------------------------
        php::string body = get("body");
        if (ctype.empty()) ctype.assign("application/x-www-form-urlencoded", 33);
        if (body.empty()) {
            // TODO 空 BODY 的处理流程？
        }
        else if (body.instanceof(php::class_entry<client_body>::entry())) {
            // TODO multipart support
        }
        else {
            body = ctype_encode(ctype, body);
            // 注意: CURLOPT_POSTFIELDS 仅"引用" body 数据
            set("body", body);
            // curl_easy_setopt(c_easy_, CURLOPT_POST, 1);
            curl_easy_setopt(c_easy_, CURLOPT_POSTFIELDSIZE, body.size());
            curl_easy_setopt(c_easy_, CURLOPT_POSTFIELDS, body.c_str());
        }
    }
}
